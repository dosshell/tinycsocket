import xml.etree.ElementTree as ET
import collections

file = r"./_doxyxml/tinycsocket_8h.xml"
FunctionData = collections.namedtuple('FunctionData', ['name', 'refid', 'description'])


def get_function_descriptons():
    m = []
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


def write_table(m: dict):
    h = f""".. csv-table::
    :header: "symbol", "description"
    :widths: 20, 20

"""
    f = open("function_index.rst", "w+")
    f.write(h)
    for n in m:
        f.write(f'''    "`{n.name} <#{n.refid}>`_", "{n.description}"\n''')


def run():
    m = get_function_descriptons()
    write_table(m)


if __name__ == '__main__':
    run()
