on run argv
    set diskImage to item 1 of argv

    tell application "Finder"
        activate
        tell disk diskImage
            set retries to 10
            repeat
                try
                    open
                    delay 1
                    set current view of container window to icon view
                    set toolbar visible of container window to false
                    set statusbar visible of container window to false
                    set the bounds of container window to {400, 100, 1000, 558}
                    set theViewOptions to the icon view options of container window
                    set arrangement of theViewOptions to not arranged
                    set icon size of theViewOptions to 72
                    set background picture of theViewOptions to file ".background:background.tiff"
                    set position of item "Tenacity" of container window to {170, 350}
                    set position of item "Applications" of container window to {430, 350}
                    close
                    open
                    update without registering applications
                    exit repeat
                on error errMsg number errNum
                    set retries to retries - 1
                    if retries is less than or equal to 0 then
                        error "DMGSetup gave up after 10 attempts: " & errMsg number errNum
                    end if
                    try
                        close
                    end try
                    delay 2
                end try
            end repeat
        end tell
    end tell
end run
