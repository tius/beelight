Import("env")

# apply this warning suppression only to C++ compilation units
env.AppendUnique(CXXFLAGS=["-Wno-volatile"])
