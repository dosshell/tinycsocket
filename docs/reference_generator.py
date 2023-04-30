# Copyright 2023 Markus Lindelöw
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files(the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import xml.etree.ElementTree as ET
import collections
from pathlib import Path
import os


FunctionData = collections.namedtuple(
    "FunctionData",
    [
        "name",
        "refid",
        "brief",
        "argsstring",
        "definition",
        "params",
        "returns",
        "seealso",
    ],
)


SymbolData = collections.namedtuple("SymbolData", ["name", "brief"])


def xml_to_rst(element):
    if element is None:
        return "<documentation is not available>"
    else:
        return "".join(element.itertext()).replace('"', '""')


def get_function_descriptions(doxygen_folder: Path):
    index_file = Path(doxygen_folder, "index.xml")
    index_root = ET.parse(index_file)
    index_files = index_root.findall(".//*[@kind='file']")
    all_files = [Path(doxygen_folder, x.get("refid") + ".xml") for x in index_files]

    m = []
    for file in all_files:
        tree = ET.parse(file)
        root = tree.getroot()
        fn = root.findall(".//*[@kind='function']")

        for f in fn:
            # symbol
            name = f.find("name").text
            brief_node = f.find("briefdescription").find("para")
            brief = brief_node.text if brief_node is not None else ""
            refid = f.get("id")
            argsstring = f.find("argsstring").text
            definition = f.find("definition").text

            # parameters
            param_nodes = f.findall("param")
            param_description_nodes = f.findall(
                "./detaileddescription//parameteritem/parameterdescription/para"
            )
            param_descriptions = [xml_to_rst(x) for x in param_description_nodes]
            param_description_name_nodes = f.findall(
                "./detaileddescription//parameteritem/parameternamelist/parametername"
            )
            param_description_names = [x.text for x in param_description_name_nodes]

            params_symbols = []
            for p in param_nodes:
                declname_node = p.find("declname")
                declname = declname_node.text if declname_node is not None else None
                typetext = xml_to_rst(p.find("type"))
                params_symbols.append((typetext, declname))

            params = []
            for p in params_symbols:
                if p[1] in param_description_names:
                    params.append(
                        (
                            p[0],
                            p[1],
                            param_descriptions[param_description_names.index(p[1])],
                        )
                    )
                else:
                    params.append((p[0], p[1], None))

            # returns
            returns = xml_to_rst(f.find(".//*[@kind='return']/para"))

            # se also
            seealso = [xml_to_rst(x) for x in f.findall(".//*[@kind='see']/para")]

            m.append(
                FunctionData(
                    name, refid, brief, argsstring, definition, params, returns, seealso
                )
            )
    return m


def get_constant_descriptions(doxygen_folder: Path):
    index_file = Path(doxygen_folder, "index.xml")
    index_root = ET.parse(index_file)
    index_files = index_root.findall(".//*[@kind='file']")
    all_files = [Path(doxygen_folder, x.get("refid") + ".xml") for x in index_files]
    m = []
    for file in all_files:
        tree = ET.parse(file)
        root = tree.getroot()
        vars = root.findall("./compounddef/sectiondef[@kind='var']/")

        for v in vars:
            definition = v.find("./definition").text
            description_node = v.find("./detaileddescription/para")
            description = description_node.text if description_node is not None else ""
            m.append(SymbolData(definition, description))
    return m


def write_symbol_csv(m, file: Path, use_links=False):
    file.parent.mkdir(parents=True, exist_ok=True)
    f = open(file, "w")
    f.write('"symbol", "description"\n')  # headers
    if use_links:
        for n in m:
            f.write(f'"`{n.name}`_", "{n.brief}"\n')
    else:
        for n in m:
            f.write(f'"{n.name}", "{n.brief}"\n')


def write_function_references(m, file: Path):
    file.parent.mkdir(parents=True, exist_ok=True)
    f = open(file, "w")
    for n in m:
        f.write(
            f"{n.name}\n"
            f"{'-' * len(n.name)}\n"
            f"{n.brief}\n"
            "\n"
            f"``{n.definition}{n.argsstring}``\n"
            "\n"
            "**Parameters:**\n"
            "\n"
            ".. csv-table::\n"
            '    :header: "type", "name", "description"\n'
            "    :widths: auto\n"
            "\n"
        )
        # CSV list of parameters
        for p in n.params:
            f.write(f"""    "{p[0]}", "{p[1]}", "{p[2]}"\n""")
        f.write(f"\n" f"**Returns:**\n\n" f"{n.returns}\n\n")
        # See Also
        if len(n.seealso) > 0:
            f.write("**See Also:**\n\n")
            for s in n.seealso:
                f.write(f"""* {s}\n""")
            f.write("\n")


def generate_references():
    dir = os.path.dirname(__file__)
    m = get_function_descriptions(Path(dir, "./_doxyxml/"))
    write_symbol_csv(m, Path(dir, "./_mybuild/function_index.csv"), use_links=True)
    write_function_references(m, Path(dir, "./_mybuild/function_references.rst"))

    c = get_constant_descriptions(Path(dir, "./_doxyxml/"))
    write_symbol_csv(c, Path(dir, "./_mybuild/variable_index.csv"))


if __name__ == "__main__":
    generate_references()
