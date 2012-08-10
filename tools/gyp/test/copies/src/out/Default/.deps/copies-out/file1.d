cmd_copies-out/file1 := ln -f "file1" "copies-out/file1" 2>/dev/null || (rm -rf "copies-out/file1" && cp -af "file1" "copies-out/file1")
