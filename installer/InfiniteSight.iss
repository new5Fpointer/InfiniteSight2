; InfiniteSight 安装程序 — Inno Setup 6 脚本
; 编译: iscc.exe "installer\InfiniteSight.iss"

#define MyAppName "InfiniteSight"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "InfiniteSight"
#define MyAppURL "https://github.com/InfiniteSight"
#define MyAppExeName "InfiniteSight.exe"

#define BuildDir "..\out\build\release"
#define VcRedistUrl "https://aka.ms/vs/17/release/vc_redist.x64.exe"

[Setup]
; 基本信息
AppId={{B8E5C3A1-4D2F-4F6A-9E8B-7C1D3A5B2E4F}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}

; 64 位应用
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

; 安装目录
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes

; 输出
OutputDir=Output
OutputBaseFilename=InfiniteSight-Setup-{#MyAppVersion}

; 压缩
Compression=lzma2/ultra64
SolidCompression=yes
LZMAUseSeparateProcess=yes
DiskSpanning=no

; 权限
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog

; 安装更新
UsePreviousAppDir=yes
UsePreviousGroup=yes

; 图标（使用程序自身的图标）
UninstallDisplayIcon={app}\{#MyAppExeName}

; Windows 版本
MinVersion=10.0.17763

[Languages]
Name: "chinesesimplified"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "快捷方式:"; Flags: checkedonce

[Files]
; 主程序
Source: "{#BuildDir}\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion

; Qt 核心 DLL
Source: "{#BuildDir}\Qt6Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\Qt6Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\Qt6Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\Qt6Network.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\Qt6Svg.dll"; DestDir: "{app}"; Flags: ignoreversion

; ICU / OpenGL 运行时
Source: "{#BuildDir}\icuuc.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\opengl32sw.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\d3dcompiler_47.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\dxcompiler.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\dxil.dll"; DestDir: "{app}"; Flags: ignoreversion

; VIPS 核心
Source: "{#BuildDir}\vips-42.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\vips-cpp-42.dll"; DestDir: "{app}"; Flags: ignoreversion

; GLib 运行时
Source: "{#BuildDir}\glib-2.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\gobject-2.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\gmodule-2.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\gio-2.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\gthread-2.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\girepository-2.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\gdk_pixbuf-2.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\ffi-8.dll"; DestDir: "{app}"; Flags: ignoreversion

; 国际化
Source: "{#BuildDir}\iconv-2.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\intl-8.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\charset-1.dll"; DestDir: "{app}"; Flags: ignoreversion

; 图像编解码器
Source: "{#BuildDir}\libpng16.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\jpeg62.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\turbojpeg.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\tiff.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\libwebp.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\libwebpdecoder.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\libwebpdemux.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\libwebpmux.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\libsharpyuv.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\exif-12.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\rsvg-2-2.dll"; DestDir: "{app}"; Flags: ignoreversion

; 字体和文本渲染
Source: "{#BuildDir}\freetype.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\fontconfig-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\harfbuzz.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\harfbuzz-gpu.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\harfbuzz-raster.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\harfbuzz-subset.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\harfbuzz-vector.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\fribidi-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\pango-1.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\pangocairo-1.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\pangoft2-1.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\pangowin32-1.0-0.dll"; DestDir: "{app}"; Flags: ignoreversion

; Cairo 渲染
Source: "{#BuildDir}\cairo-2.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\cairo-gobject-2.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\cairo-script-interpreter-2.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\pixman-1-0.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\croco-0.6-3.dll"; DestDir: "{app}"; Flags: ignoreversion

; 压缩库
Source: "{#BuildDir}\z.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\bz2.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\liblzma.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\libexpat.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\brotlicommon.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\brotlidec.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\brotlienc.dll"; DestDir: "{app}"; Flags: ignoreversion

; 数学库
Source: "{#BuildDir}\fftw3.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\fftw3f.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\fftw3l.dll"; DestDir: "{app}"; Flags: ignoreversion

; 其他
Source: "{#BuildDir}\pcre2-8.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\pcre2-16.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\pcre2-32.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\pcre2-posix.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\pthreadVC3.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\pthreadVCE3.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\pthreadVSE3.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\lcms2-2.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\libxml2.dll"; DestDir: "{app}"; Flags: ignoreversion

; Qt 插件
Source: "{#BuildDir}\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs
Source: "{#BuildDir}\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs
Source: "{#BuildDir}\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs
Source: "{#BuildDir}\iconengines\*"; DestDir: "{app}\iconengines"; Flags: ignoreversion recursesubdirs
Source: "{#BuildDir}\generic\*"; DestDir: "{app}\generic"; Flags: ignoreversion recursesubdirs
Source: "{#BuildDir}\tls\*"; DestDir: "{app}\tls"; Flags: ignoreversion recursesubdirs
Source: "{#BuildDir}\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: ignoreversion recursesubdirs

; 许可证
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
; 安装 VC++ Redist（可选静默安装）
; 取消下面一行的注释以自动静默安装 VC++ Redist
; 需要在 [Files] 节添加 Source: "vc_redist.x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall
; Filename: "{tmp}\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "正在安装 VC++ 运行库..."

; 安装完成后运行
Filename: "{app}\{#MyAppExeName}"; Description: "启动 InfiniteSight"; Flags: postinstall nowait skipifsilent; WorkingDir: "{app}"

[UninstallRun]
Filename: "{app}\{#MyAppExeName}"; Parameters: "--uninstall"; RunOnceId: "InfiniteSightUninstall"

[Code]
// 检查 VC++ Redistributable 是否已安装
function IsVCRedistInstalled: Boolean;
var
  Installed: Cardinal;
  ResultCode: Integer;
begin
  Result := False;
  // 检查 VS 2022 VC++ Redist (14.4x)
  if RegQueryDWordValue(HKLM, 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64', 'Installed', Installed) then
  begin
    Result := Installed = 1;
  end;
end;

function InitializeSetup: Boolean;
begin
  Result := True;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: Integer;
begin
  Result := '';
  if not IsVCRedistInstalled then
  begin
    if MsgBox('需要安装 Microsoft Visual C++ Redistributable 运行库。是否立即下载安装？',
              mbConfirmation, MB_YESNO) = IDYES then
    begin
      // 下载并静默安装 VC++ Redist
      if ShellExec('open', 'https://aka.ms/vs/17/release/vc_redist.x64.exe',
                   '', '', SW_SHOW, ewNoWait, ResultCode) then
      begin
        Result := '请在浏览器下载完成后安装 vc_redist.x64.exe，然后再继续安装 InfiniteSight。';
      end;
    end;
  end;
end;
