// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0

#include "vtkEncoderFactory.h"

#include "vtkObjectFactory.h"
#include "vtkVideoEncoder.h"

#include <algorithm>
#include <cctype>
#include <utility>

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkEncoderFactory);

namespace
{
// An ordered list of preferences: each entry is a key with its ranked values, both kept
// in the order the user specified them (strongest first). Keys are lowercased; values keep
// their original case.
using PreferenceList = std::vector<std::pair<std::string, std::vector<std::string>>>;

//------------------------------------------------------------------------------
// Process-wide registry and preferences, held as function-local statics so they are
// constructed on first use. A file-scope static would risk being constructed after a
// backend's static initializer runs (initialization order across shared libraries is
// unspecified), which is exactly when RegisterBackend is first called.
std::vector<vtkEncoderFactory::BackendDescriptor>& Registry()
{
  static std::vector<vtkEncoderFactory::BackendDescriptor> registry;
  return registry;
}

PreferenceList& Preferences()
{
  static PreferenceList preferences;
  return preferences;
}

//------------------------------------------------------------------------------
std::string Trim(const std::string& s)
{
  const auto notSpace = [](unsigned char c) { return std::isspace(c) == 0; };
  auto begin = std::find_if(s.begin(), s.end(), notSpace);
  auto end = std::find_if(s.rbegin(), s.rend(), notSpace).base();
  return (begin < end) ? std::string(begin, end) : std::string();
}

std::string ToLower(std::string s)
{
  std::transform(s.begin(), s.end(), s.begin(),
    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return s;
}

bool CaseInsensitiveEqual(const std::string& a, const std::string& b)
{
  return ToLower(a) == ToLower(b);
}

//------------------------------------------------------------------------------
// Normalize a codec spelling for comparison: lowercase, drop a leading "vtkvc_", and
// remove '.' so that "H.265", "H265", "VTKVC_H265" all collapse to "h265".
std::string NormalizeCodec(const std::string& raw)
{
  std::string s = ToLower(Trim(raw));
  const std::string prefix = "vtkvc_";
  if (s.rfind(prefix, 0) == 0)
  {
    s = s.substr(prefix.size());
  }
  s.erase(std::remove(s.begin(), s.end(), '.'), s.end());
  // A couple of friendly aliases.
  if (s == "hevc")
  {
    return "h265";
  }
  if (s == "avc")
  {
    return "h264";
  }
  return s;
}

// Map a codec name to the enum. Returns true and sets @a codec on success. Compares the
// normalized input against the normalized vtkVideoCodecTypeUtilities::ToString of every
// enum value, so adding a codec to Core needs no change here.
bool CodecFromString(const std::string& raw, VTKVideoCodecType& codec)
{
  const std::string want = NormalizeCodec(raw);
  if (want.empty())
  {
    return false;
  }
  for (int i = 0; i < VTKVideoCodecType::VTKVC_MaxNumberOfSupportedCodecs; ++i)
  {
    const auto candidate = static_cast<VTKVideoCodecType>(i);
    if (NormalizeCodec(vtkVideoCodecTypeUtilities::ToString(candidate)) == want)
    {
      codec = candidate;
      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------
bool SupportsCodec(const vtkEncoderFactory::BackendDescriptor& d, VTKVideoCodecType codec)
{
  return std::find(d.Codecs.begin(), d.Codecs.end(), codec) != d.Codecs.end();
}

bool BoolOf(const std::string& s)
{
  const std::string v = ToLower(Trim(s));
  return v == "true" || v == "1" || v == "yes" || v == "on";
}

//------------------------------------------------------------------------------
// Rank of a backend against one preference key's ordered value list (lower is better):
//   explicit match at value index j       -> j
//   attribute not declared by the backend -> WILDCARD (== values.size()): compatible, but
//                                            ranked after any explicit match
//   no listed value matches               -> NO_MATCH (== values.size() + 1): worst
int RankForKey(const vtkEncoderFactory::BackendDescriptor& d, const std::string& key,
  const std::vector<std::string>& values)
{
  const int wildcard = static_cast<int>(values.size());
  const int noMatch = wildcard + 1;

  if (key == "codec")
  {
    // Every backend declares its codecs. Unresolvable values (e.g. a stray "hardware=true"
    // parsed as a codec) simply never match.
    for (int j = 0; j < static_cast<int>(values.size()); ++j)
    {
      VTKVideoCodecType codec;
      if (CodecFromString(values[j], codec) && SupportsCodec(d, codec))
      {
        return j;
      }
    }
    return noMatch;
  }
  if (key == "hardware")
  {
    // Every backend declares Hardware, so there is no wildcard case here.
    for (int j = 0; j < static_cast<int>(values.size()); ++j)
    {
      if (BoolOf(values[j]) == d.Hardware)
      {
        return j;
      }
    }
    return noMatch;
  }
  // Generic attribute (e.g. "platform"). Attribute keys are stored in their original case,
  // so match them case-insensitively.
  const std::string* declared = nullptr;
  for (const auto& attribute : d.Attributes)
  {
    if (CaseInsensitiveEqual(attribute.first, key))
    {
      declared = &attribute.second;
      break;
    }
  }
  if (declared == nullptr)
  {
    return wildcard; // backend does not constrain this attribute -> compatible with any value
  }
  for (int j = 0; j < static_cast<int>(values.size()); ++j)
  {
    if (CaseInsensitiveEqual(*declared, values[j]))
    {
      return j;
    }
  }
  return noMatch;
}

// Per-key ranks for a backend, in preference order. Compared lexicographically (std::vector
// provides that via operator<): the first (strongest) key dominates, later keys break ties.
std::vector<int> ScoreBackend(
  const vtkEncoderFactory::BackendDescriptor& d, const PreferenceList& prefs)
{
  std::vector<int> score;
  score.reserve(prefs.size());
  for (const auto& pref : prefs)
  {
    score.push_back(RankForKey(d, pref.first, pref.second));
  }
  return score;
}
} // anonymous namespace

//------------------------------------------------------------------------------
void vtkEncoderFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Registered backends: " << GetNumberOfRegisteredBackends() << '\n';
  for (const auto& d : Registry())
  {
    os << indent << "  " << d.SubclassName << " (hardware=" << d.Hardware << ")\n";
  }
  os << indent << "Preferences:\n";
  for (const auto& pref : Preferences())
  {
    os << indent << "  " << pref.first << " =";
    for (const auto& value : pref.second)
    {
      os << ' ' << value;
    }
    os << '\n';
  }
}

//------------------------------------------------------------------------------
void vtkEncoderFactory::RegisterBackend(const BackendDescriptor& descriptor)
{
  auto& registry = Registry();
  for (const auto& existing : registry)
  {
    if (existing.SubclassName == descriptor.SubclassName)
    {
      return; // idempotent: guard against a module being loaded more than once
    }
  }
  registry.push_back(descriptor);
}

//------------------------------------------------------------------------------
int vtkEncoderFactory::GetNumberOfRegisteredBackends()
{
  return static_cast<int>(Registry().size());
}

//------------------------------------------------------------------------------
bool vtkEncoderFactory::CheckAvailability(VTKVideoCodecType codec)
{
  for (const auto& d : Registry())
  {
    if (!SupportsCodec(d, codec))
    {
      continue;
    }
    if (d.Available == nullptr || d.Available())
    {
      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------
void vtkEncoderFactory::SetPreferences(const char* preferences)
{
  auto& prefs = Preferences();
  prefs.clear();
  if (preferences == nullptr)
  {
    return;
  }
  // Format: "key1=v1a,v1b,...;key2=v2a,...;...". ';' separates keys (ordered strongest to
  // weakest); ',' separates a key's values (also ordered strongest to weakest).
  const std::string s(preferences);
  std::string::size_type start = 0;
  while (start <= s.size())
  {
    const std::string::size_type semicolon = s.find(';', start);
    const std::string group =
      s.substr(start, semicolon == std::string::npos ? std::string::npos : semicolon - start);
    const std::string::size_type eq = group.find('=');
    if (eq != std::string::npos)
    {
      const std::string key = ToLower(Trim(group.substr(0, eq)));
      if (!key.empty())
      {
        std::vector<std::string> values;
        const std::string valueList = group.substr(eq + 1);
        std::string::size_type vstart = 0;
        while (vstart <= valueList.size())
        {
          const std::string::size_type comma = valueList.find(',', vstart);
          std::string value = Trim(valueList.substr(
            vstart, comma == std::string::npos ? std::string::npos : comma - vstart));
          if (!value.empty()) // keep value case (e.g. Platform names)
          {
            values.push_back(value);
          }
          if (comma == std::string::npos)
          {
            break;
          }
          vstart = comma + 1;
        }
        // A repeated key replaces in place, keeping its first position.
        bool replaced = false;
        for (auto& existing : prefs)
        {
          if (existing.first == key)
          {
            existing.second = values;
            replaced = true;
            break;
          }
        }
        if (!replaced)
        {
          prefs.emplace_back(key, values);
        }
      }
    }
    if (semicolon == std::string::npos)
    {
      break;
    }
    start = semicolon + 1;
  }
}

//------------------------------------------------------------------------------
void vtkEncoderFactory::ClearPreferences()
{
  Preferences().clear();
}

//------------------------------------------------------------------------------
vtkVideoEncoder* vtkEncoderFactory::CreateEncoder()
{
  const auto& prefs = Preferences();
  auto& registry = Registry();

  // Rank every backend by preference first (no availability probe: ranking is pure attribute
  // comparison), best (lowest score) first.
  std::vector<const BackendDescriptor*> ranked;
  ranked.reserve(registry.size());
  for (const auto& d : registry)
  {
    ranked.push_back(&d);
  }
  std::stable_sort(ranked.begin(), ranked.end(),
    [&prefs](const BackendDescriptor* a, const BackendDescriptor* b)
    {
      const std::vector<int> sa = ScoreBackend(*a, prefs);
      const std::vector<int> sb = ScoreBackend(*b, prefs);
      if (sa != sb)
      {
        return sa < sb; // lexicographic: strongest key dominates, lower rank is better
      }
      if (a->Hardware != b->Hardware)
      {
        return a->Hardware; // deterministic tiebreak: prefer hardware
      }
      return a->SubclassName < b->SubclassName; // then alphabetical
    });

  // Availability is the only hard requirement: probe in rank order and take the first
  // available backend. This runs each backend's probe at most once, and only until a winner
  // is found (so e.g. NVENC's GL-window probe runs only if it out-ranks everything).
  const BackendDescriptor* best = nullptr;
  for (const auto* d : ranked)
  {
    if (d->Available == nullptr || d->Available())
    {
      best = d;
      break;
    }
  }
  if (best == nullptr)
  {
    return nullptr;
  }

  vtkVideoEncoder* encoder = best->Create();
  // Configure the encoder with the strongest preferred codec it supports. Without this the
  // encoder keeps its default codec (VP9), which a backend picked for H.264/H.265 rejects.
  if (encoder != nullptr)
  {
    for (const auto& pref : prefs)
    {
      if (pref.first != "codec")
      {
        continue;
      }
      for (const auto& value : pref.second)
      {
        VTKVideoCodecType codec;
        if (CodecFromString(value, codec) && SupportsCodec(*best, codec))
        {
          encoder->SetCodec(codec);
          break;
        }
      }
      break;
    }
  }
  return encoder;
}
