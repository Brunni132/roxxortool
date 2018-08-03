SET PATH=C:\Windows\Microsoft.NET\Framework\v4.0.30319;%PATH%
cd Code
SET VCTargetsPath=C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\V120
msbuild RoxxorTool.sln /t:Build /p:Configuration=Release /p:Platform="x64"
cd ..
