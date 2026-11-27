# Try_Agent

## qwen_agent

    运行过程：
        直接打开指定环境，运行 python ./qwen_main.py

## agent_harness

    运行过程： 
        $env:Path = "D:\Qt6\Tools\mingw1510_64\bin;D:\Qt6\6.12.1\mingw_64\bin;" + $env:Path

        cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

        cmake --build . --parallel

        .\main.exe