# Infinisat: batch STEP -> FBX/OBJ converter. Runs once on Fusion start-up, only touches its own new documents.
import adsk.core, adsk.fusion, traceback, os, threading, time

IN = r'C:\Dev\Infinisat\_downloads\cad\in'
OUT = r'C:\Dev\Infinisat\_downloads\cad\out'
EVENT_ID = 'InfinisatCADConvert'
_handlers = []
_app = None


def log(msg):
    with open(os.path.join(OUT, 'log.txt'), 'a', encoding='utf-8') as f:
        f.write(time.strftime('%H:%M:%S ') + msg + '\n')


def convert():
    app = adsk.core.Application.get()
    for fn in sorted(os.listdir(IN)):
        if not fn.lower().endswith('.step'):
            continue
        name = fn[:-5]
        if os.path.exists(os.path.join(OUT, name + '.fbx')) or os.path.exists(os.path.join(OUT, name + '.obj')):
            continue
        doc = None
        try:
            doc = app.documents.add(adsk.core.DocumentTypes.FusionDesignDocumentType)
            design = adsk.fusion.Design.cast(app.activeProduct)
            im = app.importManager
            im.importToTarget(im.createSTEPImportOptions(os.path.join(IN, fn)), design.rootComponent)
            em = design.exportManager
            try:
                em.execute(em.createFBXExportOptions(design.rootComponent, os.path.join(OUT, name + '.fbx')))
                log('ok fbx ' + name)
            except Exception:
                em.execute(em.createOBJExportOptions(design.rootComponent, os.path.join(OUT, name + '.obj')))
                log('ok obj ' + name)
        except Exception:
            log('FAIL ' + name + ' ' + traceback.format_exc().replace('\n', ' | '))
        finally:
            try:
                if doc:
                    doc.close(False)
            except Exception:
                pass
    log('DONE')
    build_library()


LIB_PROJECT = 'Infinisat Parts Library'
CATEGORY = {
    'esp32_wroom32': 'Modules', 'radio_zeta433': 'Modules',
    'dht11': 'Sensors',
    'battery_2xaa': 'Power', 'battery_18650': 'Power',
    'header_1x15': 'Connectors & Switches', 'jst_ph2': 'Connectors & Switches', 'switch': 'Connectors & Switches', 'usb_micro': 'Connectors & Switches',
    'crystal': 'Indicators & Passives', 'led_5mm': 'Indicators & Passives', 'buzzer': 'Indicators & Passives',
}
NOTE = 'KiCad 3D model library (CC-BY-SA 4.0 with KiCad exception), imported by the Infinisat game team.'


def _folder(parent, name):
    for i in range(parent.dataFolders.count):
        f = parent.dataFolders.item(i)
        if f.name == name:
            return f
    return parent.dataFolders.add(name)


def build_library():
    """Save every STEP part as a Fusion design in a shared cloud project, sorted into category folders."""
    app = adsk.core.Application.get()
    done_file = os.path.join(OUT, 'library.txt')
    done = set(open(done_file).read().split()) if os.path.exists(done_file) else set()
    proj = None
    for i in range(app.data.dataProjects.count):
        p = app.data.dataProjects.item(i)
        if p.name == LIB_PROJECT:
            proj = p
    if proj is None:
        proj = app.data.dataProjects.add(LIB_PROJECT)
        log('created project ' + LIB_PROJECT)
    for fn in sorted(os.listdir(IN)):
        if not fn.lower().endswith('.step'):
            continue
        name = fn[:-5]
        if name in done:
            continue
        doc = None
        try:
            folder = _folder(proj.rootFolder, CATEGORY.get(name, 'Misc'))
            doc = app.documents.add(adsk.core.DocumentTypes.FusionDesignDocumentType)
            design = adsk.fusion.Design.cast(app.activeProduct)
            im = app.importManager
            im.importToTarget(im.createSTEPImportOptions(os.path.join(IN, fn)), design.rootComponent)
            nice = name.replace('_', ' ').upper() if len(name) < 6 else name.replace('_', ' ').title()
            doc.saveAs(nice, folder, NOTE, 'infinisat')
            for _ in range(600):           # wait (max ~60 s) for the upload to register before closing
                adsk.doEvents()
                if doc.isSaved:
                    break
                time.sleep(0.1)
            with open(done_file, 'a') as f:
                f.write(name + chr(10))
            log('library ' + CATEGORY.get(name, 'Misc') + ' / ' + nice)
        except Exception:
            log('LIB FAIL ' + name + ' ' + traceback.format_exc().replace(chr(10), ' | '))
        finally:
            try:
                if doc:
                    doc.close(False)
            except Exception:
                pass
    log('LIBRARY DONE')


class _Handler(adsk.core.CustomEventHandler):
    def notify(self, args):
        try:
            convert()
        except Exception:
            log('ERROR ' + traceback.format_exc())


def _delayed():
    time.sleep(25)   # let Fusion finish loading the UI and sign-in
    adsk.core.Application.get().fireCustomEvent(EVENT_ID, '')


def run(context):
    try:
        os.makedirs(OUT, exist_ok=True)
        pending = [f for f in os.listdir(IN) if f.lower().endswith('.step')]
        if not pending:
            return
        app = adsk.core.Application.get()
        ev = app.registerCustomEvent(EVENT_ID)
        h = _Handler()
        ev.add(h)
        _handlers.append(h)
        log('add-in started, %d files queued' % len(pending))
        threading.Thread(target=_delayed, daemon=True).start()
    except Exception:
        log('RUN ERROR ' + traceback.format_exc())


def stop(context):
    try:
        adsk.core.Application.get().unregisterCustomEvent(EVENT_ID)
    except Exception:
        pass
