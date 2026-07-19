# MR CLI FOR YT DLP
**MR CLI FOR YT DLP** is a command-line interface wrapper for original yt-dlp.

[![Version](https://img.shields.io/badge/version-1.06-green.svg)]()
[![Platform](https://img.shields.io/badge/platform-Windows-green.svg)]()
[![License](https://img.shields.io/badge/license-MIT-green.svg)]()


## 🎯 Program Features
- Download individual videos and entire playlists
- Select start and end items for playlist downloads
- Resume interrupted playlist downloads
- Configure download parameters (location, resolution, frame rate, codec)
- Automatic format selection if the video does not match your settings
- Video transcoding (codec recompiler) – planned for the future.

## 📋 Prerequisites
- Windows 10/11 (The versions below—please test them)
- [yt-dlp](https://github.com/yt-dlp/yt-dlp) in PATH (auto-installed if missing)
- [FFmpeg](https://ffmpeg.org/) in PATH (auto-installed if missing)

## 📥 Installation
1. Download the latest release
2. Run `MR-CLI-FOR-YT-DLP.exe`
3. The program will automatically check and install required dependencies

## 🎬 Download Process
1. Enter the video or playlist URL
2. For playlists, specify start/end indices (optional)
3. Watch real-time download progress
4. File saved to your specified location
- (default: \Documents\MR-CLI-FOR-YT-DLP\)

## 🎵 Download Only Audio
1. Settings > Only audio: [ON]
2. Start download > [video URL]
3. Result: "Video Title [only audio].m4a"
4. File saved to your specified location
- (default: \Documents\MR-CLI-FOR-YT-DLP\)

## 🎦 Download Video Audio
1. Settings > Only video: [ON]
2. Start download > [video URL]
3. Result: "Video Title [only video].mp4"
4. File saved to your specified location
- (default: \Documents\MR-CLI-FOR-YT-DLP\)

## ⚙️ Settings Menu
1. Download location: [C:\Users\USER\Documents\MR-CLI-FOR-YT-DLP]
2. Video quality: [1080p 60fps MP4(H.264)]
3. Audio quality: [M4A(AAC)]
4. Codec recompiler: [OFF]
5. Only audio: [OFF]
6. Only video: [OFF]
7. Update cookies
0. Exit

## 🎨 Quality Settings
- **Video resolution**: 2160p (4K), 1440p (2K), 1080p (FullHD), 720p (HD), 480p, 360p
- **Frame rate**: 60fps or 30fps
- **Video formats**: MP4(AV1), MP4(H.264), WEBM(AV1), WEBM(VP9)
- **Audio formats**: M4A(AAC), WEBM(Opus), WEBM(Vorbis)

> **Note**: For 4K (2160p) and 2K (1440p) resolutions, YouTube typically provides only AV1 and VP9 codecs in WEBM container. MP4(H.264) is not available for these resolutions. The program automatically handles this and selects the best available codec for your chosen resolution.

## 🖥️ User Interface
- Color-coded console output for better readability
- Real-time download progress bar with speed and ETA
- Interactive menu with keyboard navigation (no Enter key needed)
- Persistent settings saved in `mr-config.txt`

## 🍪 Cookie Management
The built-in cookie editor helps you access restricted content:
- Select a cookie file (.txt)
- Paste from clipboard
- Automatic Netscape format validation

> **Note**: Currently, the program stores your cookies without encryption.

## 🔗 Supported URLs
- Single videos: `https://youtube.com/watch?v=...`
- Playlists: `https://youtube.com/playlist?list=...`
- Short links: `https://youtu.be/...`

## 🛠️ Building from Source
### Requirements
- Visual Studio 2019 or later
- Windows SDK

### Compilation
- Download "Source code.zip" in [release page](https://github.com/byMr712/MR-CLI-FOR-YT-DLP/releases)
- Open file **mr-cli-yt-dlp.sln** in Visual Studio.
- Press **Ctrl+Shift+B** to build the EXE file.

## 🤝 Contributing
1. Fork the repository
2. Create your feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## 📄 License
1. MIT License – feel free to use and modify!

## 🙏 Acknowledgments
1. [yt-dlp](https://github.com/yt-dlp/yt-dlp) – The backbone of this tool
2. [FFmpeg](https://ffmpeg.org/) – For media processing
