# MR CLI FOR YT DLP

**Powerful downloader with quality control and format management**

[![Version](https://img.shields.io/badge/version-1.01-blue.svg)]()
[![Platform](https://img.shields.io/badge/platform-Windows-blue.svg)]()
[![License](https://img.shields.io/badge/license-MIT-green.svg)]()

---

## 📖 Description

**MR CLI FOR YT DLP** is a command-line interface wrapper for original yt-dlp.

---
## 🚀 Quick Start
---
### 📋 Prerequisites
- Windows 7/8/10/11
- [yt-dlp](https://github.com/yt-dlp/yt-dlp) in PATH (auto-installed if missing)
- [FFmpeg](https://ffmpeg.org/) in PATH (auto-installed if missing)
---
### 📥 Installation
1. Download the latest release
2. Run `MR-CLI-FOR-YT-DLP.exe`
3. The program will automatically check and install required dependencies
---
### 💡 Basic Usage
1. Start download
2. Settings
3. Exit
---
### 🎬 Download Process
1. Enter the video or playlist URL
2. For playlists, specify start/end indices (optional)
3. Watch real-time download progress
4. File saved to your specified location
5. (default: \Documents\MR-CLI-FOR-YT-DLP\)
---
### 🎵 Download Only Audio
1. Settings > Only audio: [ON]
2. Start download > [video URL]
3. Result: "Video Title [only audio].m4a"
---
### 🎦 Download Video Audio
1. Settings > Only video: [ON]
2. Start download > [video URL]
3. Result: "Video Title [only video].m4a"
---
### ⚙️ Settings Menu
1. Download location: [C:\Users\USER\Documents\MR-CLI-FOR-YT-DLP]
2. Video quality: [1080p 60fps MP4(AV1)]
3. Audio quality: [M4A(AAC)]
4. Codec recompiler: [OFF]
5. Only audio: [OFF]
6. Only video: [OFF]
7. Update cookies
0. Exit

---
### 🎯 Download Management
- Download single videos and entire playlists
- Specify start and end indices for playlist downloads
- Resume interrupted downloads
- Automatic format selection based on your preferences
---
### 🎨 Quality Settings
- **Video resolution**: 2160p (4K), 1440p (2K), 1080p (FullHD), 720p (HD), 480p, 360p
- **Frame rate**: 60fps or 30fps
- **Video formats**: MP4(AV1), MP4(H.264), WEBM(AV1), WEBM(VP9)
- **Audio formats**: M4A(AAC), WEBM(Opus), WEBM(Vorbis)

> **Note**: For 4K (2160p) and 2K (1440p) resolutions, YouTube typically provides only AV1 and VP9 codecs in WEBM container. MP4(H.264) is not available for these resolutions. The program automatically handles this and selects the best available codec for your chosen resolution.
---
### 🔧 Advanced Options
- **Only audio** mode – download audio only with tag `[only audio]`
- **Only video** mode – download video only with tag `[only video]`
- **Codec recompiler** – automatic transcoding to the selected codec
- **Cookies support** – for accessing age-restricted or private content
---
### 🖥️ User Interface
- Color-coded console output for better readability
- Real-time download progress bar with speed and ETA
- Interactive menu with keyboard navigation (no Enter key needed)
- Persistent settings saved in `mr-config.txt`
---
### 🍪 Cookie Management
The built-in Cookie Editor helps you access restricted content:
- Select cookies file (.txt)
- Paste from clipboard
- Automatic validation of Netscape format

---

## 🔧 Technical Details

### Format Selection Logic
The program uses a smart fallback system:
1. **Exact resolution** with preferred codec
2. **Exact resolution** with any codec
3. **Lower resolution** with preferred codec
4. **Lower resolution** with any codec
5. **Best available** quality
---
### 🔗 Supported URLs
- Single videos: `https://youtube.com/watch?v=...`
- Playlists: `https://youtube.com/playlist?list=...`
- Short links: `https://youtu.be/...`

## 🛠️ Building from Source
### Requirements
- Visual Studio 2019 or later
- Windows SDK

### Compilation
```bash
# Using Visual Studio
msbuild MR-CLI-FOR-YT-DLP.sln /p:Configuration=Release

# Using command line
cl /EHsc /std:c++17 MR-CLI-FOR-YT-DLP.cpp
```

---
## 🤝 Contributing
1. Fork the repository
2. Create your feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## 📄 License
1. MIT License – feel free to use and modify!

## 🙏 Acknowledgments
1. yt-dlp – The backbone of this tool
2. FFmpeg – For media processing

