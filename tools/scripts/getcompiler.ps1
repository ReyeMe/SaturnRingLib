function convertFileSize {
    param(
        $bytes
    )

    if ($bytes -lt 1MB) {
        return "$([Math]::Round($bytes / 1KB, 2)) KB"
    }
    elseif ($bytes -lt 1GB) {
        return "$([Math]::Round($bytes / 1MB, 2)) MB"
    }
    elseif ($bytes -lt 1TB) {
        return "$([Math]::Round($bytes / 1GB, 2)) GB"
    }
}

function DownloadFile($url, $targetFile)
{
    #Load in the WebClient object.
    try {
        $Downloader = New-Object -TypeName System.Net.WebClient
    }
    catch [Exception] {
        Write-Error $_ -ErrorAction Stop
    }

    try {

        #Start the download by using WebClient.DownloadFileTaskAsync, since this lets us show progress on screen.
        $FileDownload = $Downloader.DownloadFileTaskAsync($Url, $targetFile)

        #Register the event from WebClient.DownloadProgressChanged to monitor download progress.
        Register-ObjectEvent -InputObject $Downloader -EventName DownloadProgressChanged -SourceIdentifier WebClient.DownloadProgressChanged | Out-Null

        #Wait two seconds for the registration to fully complete
        Start-Sleep -Seconds 3

        if ($FileDownload.IsFaulted) {
            Write-Verbose "An error occurred. Generating error."
            Write-Error $FileDownload.GetAwaiter().GetResult()
            break
        }

        #While the download is showing as not complete, we keep looping to get event data.
        while (!($FileDownload.IsCompleted)) {

            if ($FileDownload.IsFaulted) {
                Write-Error $FileDownload.GetAwaiter().GetResult()
                break
            }

            $EventData = Get-Event -SourceIdentifier WebClient.DownloadProgressChanged | Select-Object -ExpandProperty "SourceEventArgs" -Last 1

            $ReceivedData = ($EventData | Select-Object -ExpandProperty "BytesReceived")
            $TotalToReceive = ($EventData | Select-Object -ExpandProperty "TotalBytesToReceive")
            $TotalPercent = $EventData | Select-Object -ExpandProperty "ProgressPercentage"

            Write-Progress "Downloading File" -Id 2 -Status "Percent Complete: $($TotalPercent)%" -CurrentOperation "Downloaded $(convertFileSize -bytes $ReceivedData) / $(convertFileSize -bytes $TotalToReceive)" -PercentComplete $TotalPercent
        }
    }
    catch [Exception] {
        $ErrorDetails = $_

        switch ($ErrorDetails.FullyQualifiedErrorId) {
            "ArgumentNullException" { 
                Write-Error -Exception "ArgumentNullException" -ErrorId "ArgumentNullException" -Message "Either the Url or Path is null." -Category InvalidArgument -TargetObject $Downloader -ErrorAction Stop
            }
            "WebException" {
                Write-Error -Exception "WebException" -ErrorId "WebException" -Message "An error occurred while downloading the resource." -Category OperationTimeout -TargetObject $Downloader -ErrorAction Stop
            }
            "InvalidOperationException" {
                Write-Error -Exception "InvalidOperationException" -ErrorId "InvalidOperationException" -Message "The file at ""$($Path)"" is in use by another process." -Category WriteError -TargetObject $Path -ErrorAction Stop
            }
            Default {
                Write-Error $ErrorDetails -ErrorAction Stop
            }
        }
    }
    finally {
        #Cleanup tasks
        Write-Progress "Downloading File" -Id 2 -Completed
        Unregister-Event -SourceIdentifier WebClient.DownloadProgressChanged

        if (($FileDownload.IsCompleted) -and ($FileDownload.IsFaulted)) {
            #If the download was terminated, we remove the file.
            $Downloader.CancelAsync()
            Remove-Item -Path $targetFile -Force
        }

        $Downloader.Dispose()
    }
}

if ($args.Length -ne 1)
{
    write-host "Invalid parameter! Quitting..."
    Write-Host -NoNewLine 'Press any key to continue...';
    $null = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown');
    Break;
}

$folderPath = "./Compiler"

if (-not (Test-Path -Path $folderPath)) {
    New-Item -Path $folderPath -ItemType Directory
}
else {
    $directoryInfo = Get-ChildItem $folderPath | Measure-Object

    if ($directoryInfo.count -ne 0)
    {
        write-host "Compiler directory is not empty! Proceeding will clear all of its contents."
        $confirmation = Read-Host "Are you Sure You Want To Proceed (y/n)"

        if ($confirmation -eq 'y') 
        {
            write-host "Clearing compiler directory"
            Remove-Item "$($folderPath)/*" -Recurse -Force
        }
        else
        {
            Break
        }
    }
}

Write-Progress "Installing" -Id 3 -status "Step 1/3: Downloading compiler..." -PercentComplete 0
$tag="gcc_$($args[0])"

if ($args[0].equals("14.2.0")) {
    $tag="gcc_$($args[0])_2"
}

DownloadFile "https://github.com/willll/Saturn-SDK-GCC-SH2/releases/download/$($tag)/sh-gcc-$($args[0]).zip" "$($folderPath)/sh-gcc-$($args[0]).zip"

if ([System.IO.File]::Exists("$($folderPath)/sh-gcc-$($args[0]).zip")) {
    Write-Progress "Installing" -Id 3 -status "Step 2/3: Extracting compiler..." -PercentComplete 33
    Expand-Archive "$($folderPath)/sh-gcc-$($args[0]).zip" -DestinationPath "$($folderPath)"
    
    Write-Progress "Installing" -Id 3 -status "Step 3/3: Cleaning up..." -PercentComplete 66
    Remove-Item "$($folderPath)/sh-gcc-$($args[0]).zip" -Force
    
    Write-Progress "Installing" -Id 3 -status "Installation successful!" -Completed
    Write-Host "Installation successful!";
}
else {
    Write-Progress "Installing" -Id 3 -status "Installation failed!" -Completed
    Write-Host "Installation failed!";
}

Write-Host -NoNewLine 'Press any key to continue...';
$null = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown');