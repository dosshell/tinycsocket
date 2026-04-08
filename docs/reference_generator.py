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
        "detailed",
        "notes",
        "retvals",
        "argsstring",
        "definition",
        "params",
        "returns",
        "seealso",
    ],
)


SymbolData = collections.namedtuple("SymbolData", ["name", "brief"])

_SKIP_FUNCTIONS = {"tcs_static_assert"}


def _format_ref(text, linkable):
    """Format a doxygen ref as RST. Functions get links, constants get monospace."""
    if linkable and text.endswith("()"):
        anchor = text.rstrip("()")
        return f"`{text} <{anchor}_>`_"
    return f"``{text}``"


def xml_node_to_rst(node, linkable=True):
    """Convert a doxygen XML node to RST text, handling refs and special elements."""
    parts = []
    if node.text:
        parts.append(node.text)
    for child in node:
        if child.tag == "ref":
            ref_text = xml_node_to_rst(child, linkable=False)
            parts.append(_format_ref(ref_text, linkable))
        elif child.tag == "sp":
            parts.append(" ")
        else:
            parts.append(xml_node_to_rst(child, linkable=linkable))
        if child.tail:
            parts.append(child.tail)
    return "".join(parts)


def xml_to_rst(element, linkable=False):
    """Convert element to RST. linkable=False for use in CSV tables etc."""
    if element is None:
        return "<documentation is not available>"
    else:
        return xml_node_to_rst(element, linkable=linkable).replace('"', '""')


def programlisting_to_rst(element):
    """Convert a doxygen <programlisting> to an RST code block."""
    lines = []
    for codeline in element.findall("codeline"):
        line = ""
        for child in codeline:
            line += xml_node_to_rst(child, linkable=False)
        lines.append(line)
    return lines


def para_to_rst(para):
    """Convert a <para> element to RST, handling mixed text and code blocks."""
    parts = []
    if para.text and para.text.strip():
        parts.append(para.text.strip())
    for child in para:
        if child.tag == "programlisting":
            code_lines = programlisting_to_rst(child)
            parts.append("\n\n.. code-block:: c\n\n" + "\n".join("   " + l for l in code_lines) + "\n")
        elif child.tag == "ref":
            ref_text = xml_node_to_rst(child, linkable=False)
            parts.append(_format_ref(ref_text, linkable=True))
        elif child.tag == "itemizedlist":
            items = []
            for li in child.findall("listitem/para"):
                items.append(para_to_rst(li))
            parts.append("\n\n" + "\n".join(f"* {item}" for item in items) + "\n")
        elif child.tag in ("parameterlist", "simplesect"):
            pass  # handled separately
        else:
            parts.append(xml_node_to_rst(child))
        if child.tail and child.tail.strip():
            parts.append(child.tail.strip())
    return " ".join(parts) if parts else ""


def is_plain_para(para):
    """Check if a <para> element is a plain description paragraph (not params, returns, etc.)."""
    for child in para:
        if child.tag in ("parameterlist", "simplesect"):
            return False
    return True


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
            if name in _SKIP_FUNCTIONS:
                continue
            brief_node = f.find("briefdescription").find("para")
            brief = brief_node.text if brief_node is not None else ""
            refid = f.get("id")
            argsstring = f.find("argsstring").text
            definition = f.find("definition").text

            # detailed description paragraphs (excluding param/return/see/note sections)
            detailed_paras = []
            detail_node = f.find("detaileddescription")
            if detail_node is not None:
                for para in detail_node.findall("para"):
                    if is_plain_para(para):
                        text = para_to_rst(para).strip()
                        if text:
                            detailed_paras.append(text)
            detailed = "\n\n".join(detailed_paras)

            # notes
            notes = [xml_to_rst(x) for x in f.findall("./detaileddescription//simplesect[@kind='note']/para")]

            # retvals
            retvals = []
            for item in f.findall("./detaileddescription//parameterlist[@kind='retval']/parameteritem"):
                rv_name = xml_to_rst(item.find("parameternamelist/parametername"), linkable=True)
                rv_desc = xml_to_rst(item.find("parameterdescription/para"), linkable=True)
                retvals.append((rv_name, rv_desc))

            # parameters
            param_nodes = f.findall("param")
            param_desc_map = {}
            for item in f.findall("./detaileddescription//parameterlist[@kind='param']/parameteritem"):
                pname_node = item.find("parameternamelist/parametername")
                pdesc_node = item.find("parameterdescription/para")
                if pname_node is not None and pname_node.text:
                    param_desc_map[pname_node.text] = xml_to_rst(pdesc_node) if pdesc_node is not None else ""

            params = []
            for p in param_nodes:
                declname_node = p.find("declname")
                declname = declname_node.text if declname_node is not None else None
                typetext = xml_to_rst(p.find("type"))
                desc = param_desc_map.get(declname, None)
                params.append((typetext, declname, desc))

            # returns
            returns_nodes = f.findall("./detaileddescription//simplesect[@kind='return']/para")
            returns = " ".join(xml_to_rst(x, linkable=True) for x in returns_nodes) if returns_nodes else ""

            # see also
            seealso = [xml_to_rst(x, linkable=True) for x in f.findall("./detaileddescription//simplesect[@kind='see']/para")]

            m.append(
                FunctionData(
                    name, refid, brief, detailed, notes, retvals, argsstring, definition, params, returns, seealso
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
            definition = definition.replace("const ", "")
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
            ".. code-block:: c\n"
            "\n"
            f"   {n.definition}{n.argsstring}\n"
            "\n"
        )

        # Detailed description
        if n.detailed:
            f.write(f"{n.detailed}\n\n")

        # Notes
        for note in n.notes:
            f.write(f".. note::\n\n   {note}\n\n")

        # Parameters
        f.write(
            "**Parameters:**\n"
            "\n"
            ".. csv-table::\n"
            '    :header: "type", "name", "description"\n'
            "    :widths: 20, 20, 60\n"
            "    :width: 100%\n"
            "\n"
        )
        # CSV list of parameters
        for p in n.params:
            desc = str(p[2]).replace("\n", " ") if p[2] else ""
            f.write(f"""    "{p[0]}", "{p[1]}", "{desc}"\n""")

        # Returns
        if n.returns:
            f.write(f"\n**Returns:**\n\n{n.returns}\n\n")

        # Retvals
        if n.retvals:
            f.write("**Return values:**\n\n")
            for rv_name, rv_desc in n.retvals:
                f.write(f"* {rv_name} -- {rv_desc}\n")
            f.write("\n")

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
