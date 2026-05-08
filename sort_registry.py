import re

with open('src/common/config_registry.cpp', 'r') as f:
    content = f.read()

prefix = content[:content.find('    g_configRegistry.push_back')]
suffix = "}\n"

# find all push_back statements. They span exactly 2 lines each in this file.
# Wait, some might span more? Let's use regex to find each push_back
push_backs = re.findall(r'    g_configRegistry\.push_back\([^\)]+\)\);\n', content)

# wait, the regex might be tricky if there are nested parens.
# In this file, each push_back ends with OnConfigChange));
push_backs = re.findall(r'    g_configRegistry\.push_back\(.*?(?:OnConfigChange|nullptr)\)\);\n', content, re.DOTALL)

def get_section(s):
    match = re.search(r'CONFIG_[A-Z]+\(\s*"([^"]+)"', s)
    if match:
        return match.group(1)
    return ""

push_backs.sort(key=get_section)

with open('src/common/config_registry.cpp', 'w') as f:
    f.write(prefix)
    for pb in push_backs:
        f.write(pb)
    f.write(suffix)
