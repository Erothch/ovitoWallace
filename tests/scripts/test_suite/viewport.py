from ovito.io import *
from ovito.vis import *
import ovito
from PyQt5 import QtCore

# Import a data file.
pipeline = import_file("../../files/CFG/shear.void.120.cfg")
pipeline.add_to_scene()

settings = RenderSettings(size = (20,20))
settings.renderer = TachyonRenderer(ambient_occlusion = False, antialiasing = False)

vp = Viewport()
vp.type = Viewport.Type.PERSPECTIVE
vp.type = Viewport.Type.Perspective
vp.camera_pos = (350, -450, 450)
vp.camera_dir = (-100, -50, -50)
print(vp.fov)
print(vp.type)
vp.zoom_all()

overlay = TextLabelOverlay(
    text = 'Some text', 
    alignment = QtCore.Qt.AlignHCenter ^ QtCore.Qt.AlignBottom,
    offset_y = 0.1,
    font_size = 0.03,
    text_color = (0,0,0))

vp.overlays.append(overlay)
vp.render(settings)
vp.render_image(renderer=settings.renderer, size=settings.size)

for vp in ovito.scene.viewports:
    print(vp)