param(
    [Parameter(Mandatory = $true)] [string] $ExePath,
    [Parameter(Mandatory = $true)] [string] $OutFile
)

# Read the PE Machine field and classify the target as x64, arm64 or x86.
$bytes    = [IO.File]::ReadAllBytes($ExePath)
$peOffset = [BitConverter]::ToInt32($bytes, 60)
$machine  = [BitConverter]::ToUInt16($bytes, $peOffset + 4)
$arch = switch ($machine) { 0x8664 { 'x64' } 0xAA64 { 'arm64' } default { 'x86' } }

[IO.File]::WriteAllText($OutFile, $arch)
