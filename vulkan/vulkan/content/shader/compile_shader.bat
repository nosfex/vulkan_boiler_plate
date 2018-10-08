%VK_SDK_PATH%\Bin32\glslangValidator.exe -V %~dp0shader.vert
%VK_SDK_PATH%\Bin32\glslangValidator.exe -V %~dp0shader.frag
COPY "%~dp0vert.spv" "%~dp0..\..\..\x64\Debug\content\shader\vert.spv"
COPY "%~dp0frag.spv" "%~dp0..\..\..\x64\Debug\content\shader\frag.spv"