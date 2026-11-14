# Convert the UCS Satellite Database (.xlsx, free, https://www.ucs.org/resources/satellite-database) into a compact
# CSV the game loads at runtime: Content/Infinisat/Data/ucs_satellites.csv
# Run with any Python 3 (no packages needed), e.g. Unreal's bundled one:
#   "<UE>/Engine/Binaries/ThirdParty/Python3/Win64/python.exe" -I Tools/import_ucs.py "<path>/UCS-Satellite-Database 5-1-2023.xlsx"
import csv, os, re, sys, zipfile, xml.etree.ElementTree as ET

NS = {'m': 'http://schemas.openxmlformats.org/spreadsheetml/2006/main'}
WANT = [  # (output column, UCS header)
    ("name", "Current Official Name of Satellite"),
    ("country", "Country of Operator/Owner"),
    ("operator", "Operator/Owner"),
    ("users", "Users"),
    ("purpose", "Purpose"),
    ("detail", "Detailed Purpose"),
    ("orbit_class", "Class of Orbit"),
    ("orbit_type", "Type of Orbit"),
    ("geo_lon", "Longitude of GEO (degrees)"),
    ("perigee_km", "Perigee (km)"),
    ("apogee_km", "Apogee (km)"),
    ("ecc", "Eccentricity"),
    ("inc_deg", "Inclination (degrees)"),
    ("period_min", "Period (minutes)"),
    ("launch_mass_kg", "Launch Mass (kg.)"),
    ("dry_mass_kg", "Dry Mass (kg.)"),
    ("power_w", "Power (watts)"),
    ("launch_date", "Date of Launch"),
    ("life_yr", "Expected Lifetime (yrs.)"),
    ("contractor", "Contractor"),
    ("launch_site", "Launch Site"),
    ("vehicle", "Launch Vehicle"),
    ("cospar", "COSPAR Number"),
    ("norad", "NORAD Number"),
]


def col_index(ref):
    letters = re.match(r"[A-Z]+", ref).group(0)
    n = 0
    for ch in letters:
        n = n * 26 + (ord(ch) - 64)
    return n - 1


def excel_date(v):
    try:
        import datetime
        d = datetime.date(1899, 12, 30) + datetime.timedelta(days=int(float(v)))
        return d.isoformat()
    except Exception:
        return v


def main(src, dst):
    z = zipfile.ZipFile(src)
    ss = [''.join(t.text or '' for t in si.iter('{%s}t' % NS['m']))
          for si in ET.fromstring(z.read('xl/sharedStrings.xml')).findall('m:si', NS)]
    rows = ET.fromstring(z.read('xl/worksheets/sheet1.xml')).find('m:sheetData', NS).findall('m:row', NS)

    def cells(r):
        out = {}
        for c in r.findall('m:c', NS):
            v = c.find('m:v', NS)
            if v is None:
                continue
            out[col_index(c.get('r'))] = ss[int(v.text)] if c.get('t') == 's' else v.text
        return out

    head = cells(rows[0])
    idx = {h.strip(): i for i, h in head.items()}
    cols = [(o, idx[h]) for o, h in WANT]
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    n = 0
    with open(dst, 'w', newline='', encoding='utf-8') as f:
        w = csv.writer(f)
        w.writerow([o for o, _ in cols])
        for r in rows[1:]:
            c = cells(r)
            vals = [(c.get(i, '') or '').replace('\xa0', ' ').strip() for _, i in cols]
            if not vals[0]:
                continue
            di = [o for o, _ in cols].index("launch_date")
            vals[di] = excel_date(vals[di]) if vals[di] else ''
            w.writerow(vals)
            n += 1
    print("wrote", n, "satellites to", dst)


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    main(sys.argv[1], os.path.join(here, "..", "Content", "Infinisat", "Data", "ucs_satellites.csv"))
