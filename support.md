# Codec Support

When multiple encoder classes support a codec, the preferred encoders are highlighted in bold.

|Codec|Class|
|---|---|
|VP9|**vtkFFmpegSoftwareEncoder**, vtkFFmpegHardwareEncoder|
|AV1|vtkFFmpegSoftwareEncoder|
|H.264|vtkFFmpegSoftwareEncoder, vtkFFmpegHardwareEncoder, **vtkNvEncoderGL**|
|H.265|vtkFFmpegSoftwareEncoder, vtkFFmpegHardwareEncoder, **vtkNvEncoderGL**|
|JPEG|vtkJPEGVideoEncoder|

# Asynchronous encoding
When an encoder operates asynchronously, input is pushed onto a task queue for later encoding. A worker
thread picks up frames from the task queue one-by-one and sends them for encoding. After encoding, an encoder
emits `vtkCommand::ProgressEvent`. You may then call `vtkVideoEncoder::GetResult()` to obtain compressed
video packet(s). In order to sustain input until the video encoder no longer needs it,
the asynchronous mode mandates atleast 1 deep-copy of the input image pixels.

Proper hardware encoders (vtkNvEncoderGL) do **not** support asynchronous mode.
Please use `vtkVideoEncoder::EncodeDisplay()` for efficient zero-copy encoding.

# API Support
|Encoder|Async|Push(frame)|GetResult|Encode(frame)|EncodeDisplay()|
|---|---|---|---|---|---|
|`vtkFFmpegSoftwareEncoder`|`true`|yes|yes|yes|no|
|`vtkFFmpegSoftwareEncoder`|`false`|yes|yes|yes|no|
|`vtkFFmpegHardwareEncoder`|`true`|yes|yes|yes|no|
|`vtkFFmpegHardwareEncoder`|`false`|yes|yes|yes|no|
|`vtkJPEGVideoEncoder`|`true`|yes|yes|yes|no|
|`vtkJPEGVideoEncoder`|`false`|yes|yes|yes|no|
|`vtkNvEncoderGL`|`N/A`|yes|yes|yes|yes|
