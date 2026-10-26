param([string]$InstallDirectory = (Join-Path $PSScriptRoot 'install'))
$ErrorActionPreference = 'Stop'
$taskTools = Join-Path (Split-Path $PSScriptRoot -Parent) '.tools'
if(!(Test-Path -LiteralPath $taskTools)) {$taskTools=Join-Path (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent) '.tools'}
$taskCompiler = Join-Path $taskTools 'llvm-mingw-20240619-ucrt-x86_64\bin'
$taskCmake = Join-Path $taskTools 'cmake-4.4.4-windows-x86_64\bin\cmake.exe'
$taskCtest = Join-Path $taskTools 'cmake-4.4.4-windows-x86_64\bin\ctest.exe'
$taskNinja = Join-Path $taskTools 'ninja\ninja.exe'
$taskSfml = Join-Path $taskTools 'SFML-2.6.2'
if (!(Test-Path -LiteralPath $taskCmake) -or !(Test-Path -LiteralPath $taskCompiler)) {
    throw 'Bundled local tools were not found. See IMPROVEMENTS.zh-CN.md for a standard CMake/MSVC build.'
}
$taskOldPath = $env:PATH
try {
    $env:PATH = $taskCompiler + ';' + $env:PATH
    & $taskCmake -S $PSScriptRoot -B "$PSScriptRoot\build-release" -G Ninja "-DCMAKE_MAKE_PROGRAM=$taskNinja" '-DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-clang++.exe' '-DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres.exe' "-DFETCHCONTENT_SOURCE_DIR_SFML=$taskSfml" '-DCMAKE_BUILD_TYPE=Release' '-DCMAKE_POLICY_VERSION_MINIMUM=3.5' '-DBUILD_SHARED_LIBS=ON' '-DSFML_BUILD_NETWORK=OFF' '-DSFML_BUILD_EXAMPLES=OFF' '-DSFML_BUILD_DOC=OFF' '-DSCHIFFBRUCH_QA_PREVIEW=OFF'
    if ($LASTEXITCODE -ne 0) { throw 'Configuration failed.' }
    & $taskCmake --build "$PSScriptRoot\build-release" --parallel 8
    if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
    & $taskCtest --test-dir "$PSScriptRoot\build-release" --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }
    & $taskCmake --install "$PSScriptRoot\build-release" --config Release --prefix $InstallDirectory
    if ($LASTEXITCODE -ne 0) { throw 'Installation failed.' }
    foreach ($taskDll in @('libc++.dll', 'libunwind.dll', 'libwinpthread-1.dll')) {
        Copy-Item -LiteralPath (Join-Path $taskCompiler $taskDll) -Destination (Join-Path $InstallDirectory 'bin')
    }
} finally {
    $env:PATH = $taskOldPath
}
