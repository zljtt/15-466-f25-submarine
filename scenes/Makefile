.PHONY : all clean

#n.b. the '-y' sets autoexec scripts to 'on' so that driver expressions will work
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	BLENDER = /Applications/Blender.app/Contents/MacOS/Blender -y
else ifeq ($(UNAME_S),Linux)
	BLENDER = ../../blender-4.5.2-linux-x64/blender -y
else
	BLENDER = blender
endif

EXPORT_PREFAB_MESHES=export-meshes.py
EXPORT_SCENE_MESHES=export-meshes.py
EXPORT_SCENE=export-scene.py

DIST=../dist

all : \
	$(DIST)/prototype_prefab.pnct \
	$(DIST)/prototype_scene.pnct \
	$(DIST)/prototype.scene \

$(DIST)/prototype_prefab.pnct : prototype_map2.blend $(EXPORT_PREFAB_MESHES)
	$(BLENDER) --background --python $(EXPORT_PREFAB_MESHES) -- '$<':Prefab '$@'

$(DIST)/prototype_scene.pnct : prototype_map2.blend $(EXPORT_SCENE_MESHES)
	$(BLENDER) --background --python $(EXPORT_SCENE_MESHES) -- '$<':Scene '$@'

$(DIST)/prototype.scene : prototype_map2.blend $(EXPORT_SCENE)
	$(BLENDER) --background --python $(EXPORT_SCENE) -- '$<':Scene '$@'

clean :
	rm -f \
	$(DIST)/prototype_prefab.pnct \
	$(DIST)/prototype_scene.pnct \
	$(DIST)/prototype.scene
