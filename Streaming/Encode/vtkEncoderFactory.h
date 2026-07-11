// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: Apache-2.0
/**
 * @class   vtkEncoderFactory
 * @brief   TEMPORARY preference-driven selector for concrete vtkVideoEncoder backends
 *
 * vtkEncoderFactory picks the best available concrete encoder for a set of
 * key=value preferences (Codec, Hardware, Platform). It exists only because this
 * project targets VTK 9.6, which lacks the object-factory *override attribute* API
 * (vtkOverrideAttribute + vtkObjectFactory::SetPreferences) that ships in VTK 9.7.
 *
 * @warning This class is temporary scaffolding. When the project upgrades to VTK
 * 9.7, delete this class and its per-backend registrars, and instead have each
 * concrete encoder return its attributes from CreateOverrideAttributes() so that
 * vtkObjectFactory::SetPreferences() + vtkVideoEncoder::New() do the selection. The
 * BackendDescriptor::Attributes maps deliberately use the 9.7 attribute vocabulary
 * so that migration is mechanical.
 *
 * Backends register themselves (see RegisterBackend) from a static initializer in
 * their own translation unit, so this class never has to #include a backend header
 * (which would create a dependency cycle: backends DEPEND on this Encode module).
 * A backend only becomes visible after its shared library is loaded; in Python this
 * happens automatically when `vtk_streaming` is imported.
 *
 * @sa vtkVideoEncoder, vtkVideoCodecTypes
 */

#ifndef vtkEncoderFactory_h
#define vtkEncoderFactory_h

#include "vtkObject.h"

#include "vtkStreamingEncodeModule.h" // for export macro
#include "vtkVideoCodecTypes.h"       // for VTKVideoCodecType

#include <map>    // for the descriptor attribute map
#include <string> // for descriptor fields
#include <vector> // for the codec list

class vtkVideoEncoder;

class VTKSTREAMINGENCODE_EXPORT vtkEncoderFactory : public vtkObject
{
public:
  vtkTypeMacro(vtkEncoderFactory, vtkObject);
  static vtkEncoderFactory* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Raw C function pointers so a descriptor stays trivially copyable and free of
   * heap allocation. Create() returns a new encoder (VTK New() ownership); Available()
   * probes whether a usable encoder is present on this machine.
   */
  using CreateFn = vtkVideoEncoder* (*)();
  using AvailFn = bool (*)();
  ///@}

  /**
   * Everything vtkEncoderFactory needs to know about one concrete backend. Registered
   * by the backend itself so this module stays decoupled from concrete encoders.
   */
  struct BackendDescriptor
  {
    std::string SubclassName;                      // e.g. "vtkVideoToolboxEncoder"
    CreateFn Create = nullptr;                     // upcasts the concrete New() to the base
    AvailFn Available = nullptr;                   // nullptr => always available (software)
    bool Hardware = false;                         // hardware-accelerated backend?
    std::vector<VTKVideoCodecType> Codecs;         // codecs this backend supports
    std::map<std::string, std::string> Attributes; // 9.7 vocabulary: Platform, Hardware, ...
  };

  /**
   * Register a concrete backend. Called by each backend's static initializer at library
   * load time. Idempotent: a second registration with the same SubclassName is ignored,
   * so re-importing a module does not double-register.
   */
  static void RegisterBackend(const BackendDescriptor& descriptor);

  /**
   * Returns true when any registered backend supports @a codec and is available on this
   * machine. Ignores the current preferences.
   */
  static bool CheckAvailability(VTKVideoCodecType codec);

  /**
   * Set the selection preferences from a ranked, multi-value string in the VTK 9.7 override
   * format: "key1=v1a,v1b,...;key2=v2a,...;...". ';' separates keys; ',' separates a key's
   * values. Ordering is strength: keys are ranked strongest-to-weakest, and so are the values
   * within each key. e.g. "Codec=H265,H264,VP9;Hardware=true;Platform=macOS".
   *
   * Selection (see CreateEncoder) is a lexicographic *soft* ranking: availability is the only
   * hard requirement, every key is a preference, and a mismatch merely lowers rank. Put a key
   * first to make it decisive (e.g. "Hardware=true;Codec=..." makes hardware dominate codec).
   *
   * Keys are case-insensitive; whitespace is trimmed; a repeated key keeps its first position
   * with the last-specified values. Passing nullptr clears preferences. Recognized keys: Codec
   * (vp9/av1/h264/h265, tolerant of "H.264"/"HEVC" spellings), Hardware (true/false), Platform
   * (macOS/Linux/Windows). Note ';' is required between keys: a "key=value" token sitting after
   * a comma is treated as a value of the preceding key (and dropped if it does not resolve).
   */
  static void SetPreferences(const char* preferences);

  /**
   * Clear all preferences (equivalent to SetPreferences(nullptr)).
   */
  static void ClearPreferences();

  /**
   * Create the best available encoder for the current preferences, or nullptr when none
   * matches. The caller owns the returned object (standard VTK New() semantics).
   *
   * Among available backends that satisfy every set preference (codec support, Platform,
   * Hardware), hardware backends are preferred, with a deterministic alphabetical
   * SubclassName tiebreak.
   */
  static vtkVideoEncoder* CreateEncoder();

  /**
   * Number of registered backends. Mostly useful for tests/diagnostics.
   */
  static int GetNumberOfRegisteredBackends();

protected:
  vtkEncoderFactory() = default;
  ~vtkEncoderFactory() override = default;

private:
  vtkEncoderFactory(const vtkEncoderFactory&) = delete;
  void operator=(const vtkEncoderFactory&) = delete;
};

#endif // vtkEncoderFactory_h
