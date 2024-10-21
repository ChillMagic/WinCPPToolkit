$version = "3.12.3"

$url_x86_64 = "https://www.python.org/ftp/python/$version/python-$version-embed-amd64.zip"
$url_x86    = "https://www.python.org/ftp/python/$version/python-$version-embed-win32.zip"
$url_arm64  = "https://www.python.org/ftp/python/$version/python-$version-embed-arm64.zip"

if (-not $PSScriptRoot) {
    $PSScriptRoot = Split-Path -Parent -Path $MyInvocation.MyCommand.Definition
}

$extractPath = Join-Path $PSScriptRoot "python"
$toolArchMap = @{
    "x86"   = "x86"
    "AMD64" = "x86_64"
    "ARM64" = "arm64"
}
$arch = $toolArchMap[$env:PROCESSOR_ARCHITECTURE]
$curl = "$PSScriptRoot\curl\$arch\curl.exe"
$7z = "$PSScriptRoot\7zip\$arch\7z.exe"


if (Test-Path $extractPath) {
    $userChoice = Read-Host "Directory ``$extractPath`` already exists. Do you want to remove it? (y/N)"
    if ($userChoice -eq 'Y' -or $userChoice -eq 'y') {
        Remove-Item -Path $extractPath -Recurse -Force
        Write-Host "Removed existing directory: $extractPath"
    } else {
        Write-Host "Operation aborted by the user."
        exit
    }
}

function Get-RandomSuffix {
    return -join ((65..90) + (97..122) | Get-Random -Count 6 | ForEach-Object { [char]$_ })
}

function Download {
    $outfile = Join-Path $env:TEMP ("python_" + $version + "_" + (Get-RandomSuffix) + ".zip")
    $urlMap = @{
        "x86"   = $url_x86
        "AMD64" = $url_x86_64
        "ARM64" = $url_arm64
    }
    $url = $urlMap[$env:PROCESSOR_ARCHITECTURE]
    if ($null -eq $url) {
        Write-Error "Unknown architecture ``$env:PROCESSOR_ARCHITECTURE`` - script will now exit"
        exit 1
    }

    &$curl $url "-o" $outfile
    # Invoke-WebRequest -Uri $url -OutFile $outfile
    Write-Host "Downloaded to $outfile"
    return $outfile
}

function Extract {
    New-Item -ItemType Directory -Path $extractPath | Out-Null
    &$7z x $outfile "-o$extractPath"
    # Expand-Archive -Path $outfile -DestinationPath $extractPath
    Write-Host "Extracted to $extractPath"
}

$outfile = Download
Extract
