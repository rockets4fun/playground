for /R %%f in (..\Prototype\*.cpp) do clang-format.exe -verbose -style=file -fallback-style=LLVM -i "%%f"
for /R %%f in (..\Prototype\*.hpp) do clang-format.exe -verbose -style=file -fallback-style=LLVM -i "%%f"
