set_project("GBN")
set_version("1.0")

-- 指定源代码目录
add_rules("mode.debug", "mode.release")
add_includedirs("include")  -- 包含头文件目录
add_linkdirs("lib")         -- 包含库文件目录
add_links("examplelibnetsim")       -- 链接到名为"example"的库

target("GBN")
    set_kind("binary")
    add_files("src/*.cpp")
