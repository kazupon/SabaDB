cmd_out/Default/copies-out/file2 := ln -f "file2" "out/Default/copies-out/file2" 2>/dev/null || (rm -rf "out/Default/copies-out/file2" && cp -af "file2" "out/Default/copies-out/file2")
