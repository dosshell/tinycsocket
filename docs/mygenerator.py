import xml.etree.ElementTree as ET
import collections
from pathlib import Path

FunctionData = collections.namedtuple(
    'FunctionData', ['name', 'refid', 'description'])


def get_function_descriptons(doxygen_folder: Path):
    index_file = Path(doxygen_folder, 'index.xml')
    index_root = ET.parse(index_file)
    index_files = index_root.findall(".//*[@kind='file']")
    all_files = [Path(doxygen_folder, x.get('refid') + '.xml') for x in index_files]

    m = []
    for file in all_files:
        tree = ET.parse(file)
        root = tree.getroot()
        fn = root.findall(".//*[@kind='function']")

        for f in fn:
            d = f.find('name').text
            b = f.find('briefdescription').find('para')
            refid = f.get('id')

            if b is not None:
                txt = b.text
            else:
                txt = ""
            m.append(FunctionData(d, refid, txt))
    return m


def write_function_index(m, file: Path):
    file.parent.mkdir(parents=True, exist_ok=True)
    f = open(file, "w")
    f.write(""".. csv-table::
    :header: "symbol", "description"
    :widths: 20, 20

""")
    for n in m:
        f.write(f'''    "`{n.name} <#{n.refid}>`_", "{n.description}"\n''')


def run():
    m = get_function_descriptons(Path('./_doxyxml/'))
    write_function_index(m, Path('./_mybuild/function_index.rst'))


if __name__ == '__main__':
    run()
