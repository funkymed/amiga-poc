# Configuration pour Amiga 500 + Kickstart 1.3
DOCKER_IMAGE = walkero/docker4amigavbcc:latest-m68k
DOCKER_RUN = docker run --rm -v $(PWD):/work -w /work -e VBCC=/opt/vbcc $(DOCKER_IMAGE)

CC = vc
CFLAGS = +aos68k -O0 -cpu=68000 -size

# Répertoires
SRC_DIR = src
BUILD_DIR = build
DISK_SIMPLE = disk/simple
DISK_DEMO = disk/demo
ASSETS_DIR = assets

# Cibles
TARGET_SIMPLE = $(BUILD_DIR)/simple_amiga
TARGET_DEMO = $(BUILD_DIR)/amiga_demo

.DEFAULT_GOAL := help
.PHONY: help all clean simple demo test list install adf adf-simple adf-demo convert-png

help: ## Affiche cette aide
	@echo "Commandes disponibles:"
	@echo ""
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-15s\033[0m %s\n", $$1, $$2}'

all: simple ## Compile tous les projets

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

simple: $(BUILD_DIR) ## Compile le programme Hello World avec MOD
	@echo "🔨 Compilation Hello World avec lecteur MOD..."
	$(DOCKER_RUN) $(CC) +aos68k -cpu=68000 -O1 -I/opt/sdk/NDK_3.9/Include/include_h -I$(SRC_DIR) -I$(SRC_DIR)/SDI -o $(TARGET_SIMPLE) $(SRC_DIR)/main_simple.c $(SRC_DIR)/ptplayer/ptplayer.asm -lamiga
	@echo "✅ Compilation terminée: $(TARGET_SIMPLE)"
	@echo "📁 Programme Hello World avec MOD disponible dans: $(TARGET_SIMPLE)"

demo: $(BUILD_DIR) ## Compile la démo complète Amiga
	@echo "🔨 Compilation démo complète pour Amiga 500..."
	$(DOCKER_RUN) $(CC) $(CFLAGS) -o $(TARGET_DEMO) $(SRC_DIR)/main.c $(SRC_DIR)/amiga_base.c $(SRC_DIR)/display.c $(SRC_DIR)/image.c $(SRC_DIR)/audio.c -lamiga
	@echo "✅ Compilation terminée: $(TARGET_DEMO)"
	@echo "📁 Binaire disponible dans: $(TARGET_DEMO)"


test: simple ## Compile et teste l'exemple simple
	@echo "🧪 Test de compilation..."
	@if [ -f $(TARGET_SIMPLE) ]; then \
		echo "✅ Binaire créé avec succès: $(TARGET_SIMPLE)"; \
		ls -lh $(TARGET_SIMPLE); \
	else \
		echo "❌ Erreur: binaire non trouvé"; \
		exit 1; \
	fi

clean: ## Nettoie les fichiers compilés
	@echo "🧹 Nettoyage..."
	rm -rf $(BUILD_DIR) *.adf
	@echo "✅ Nettoyage terminé"

install: simple ## Installe le binaire dans /tmp
	@echo "📦 Installation..."
	cp $(TARGET_SIMPLE) /tmp/
	@echo "✅ Installé dans /tmp/simple_amiga"

adf-simple: simple ## Crée une disquette ADF simple avec MOD pour Amiga 500
	@echo "💾 Création disquette ADF simple avec MOD pour Amiga 500..."
	@rm -f $(BUILD_DIR)/simple_amiga.adf
	python3 /Users/cyrilpereira/Sites/poc/amitools/bin/xdftool $(BUILD_DIR)/simple_amiga.adf create + format 'A500-SIMPLE' ofs
	python3 /Users/cyrilpereira/Sites/poc/amitools/bin/xdftool $(BUILD_DIR)/simple_amiga.adf boot install
	python3 /Users/cyrilpereira/Sites/poc/amitools/bin/xdftool $(BUILD_DIR)/simple_amiga.adf write $(TARGET_SIMPLE)
	python3 /Users/cyrilpereira/Sites/poc/amitools/bin/xdftool $(BUILD_DIR)/simple_amiga.adf write $(ASSETS_DIR)/yeah.mod
	python3 /Users/cyrilpereira/Sites/poc/amitools/bin/xdftool $(BUILD_DIR)/simple_amiga.adf write $(ASSETS_DIR)/yeah.mod.rle
	python3 /Users/cyrilpereira/Sites/poc/amitools/bin/xdftool $(BUILD_DIR)/simple_amiga.adf write $(ASSETS_DIR)/PNG_Test_256colors.png
	python3 /Users/cyrilpereira/Sites/poc/amitools/bin/xdftool $(BUILD_DIR)/simple_amiga.adf write $(ASSETS_DIR)/fishtank_320x256x3.ts
	python3 /Users/cyrilpereira/Sites/poc/amitools/bin/xdftool $(BUILD_DIR)/simple_amiga.adf write $(ASSETS_DIR)/fishtank_320x200x3.ts
	python3 /Users/cyrilpereira/Sites/poc/amitools/bin/xdftool $(BUILD_DIR)/simple_amiga.adf makedir c
	python3 /Users/cyrilpereira/Sites/poc/amitools/bin/xdftool $(BUILD_DIR)/simple_amiga.adf write $(DISK_SIMPLE)/c/Echo c/
	python3 /Users/cyrilpereira/Sites/poc/amitools/bin/xdftool $(BUILD_DIR)/simple_amiga.adf write $(DISK_SIMPLE)/c/Execute c/
	python3 /Users/cyrilpereira/Sites/poc/amitools/bin/xdftool $(BUILD_DIR)/simple_amiga.adf write $(DISK_SIMPLE)/c/Wait c/
	python3 /Users/cyrilpereira/Sites/poc/amitools/bin/xdftool $(BUILD_DIR)/simple_amiga.adf makedir s
	python3 /Users/cyrilpereira/Sites/poc/amitools/bin/xdftool $(BUILD_DIR)/simple_amiga.adf write $(DISK_SIMPLE)/s/startup-sequence s/
	python3 /Users/cyrilpereira/Sites/poc/amitools/bin/xdftool $(BUILD_DIR)/simple_amiga.adf list
	@echo "✅ Disquette ADF simple créée: $(BUILD_DIR)/simple_amiga.adf"
	@echo "🚀 Compatible Amiga 500 + Kickstart 1.3 avec MOD"

adf-demo: demo ## Crée une disquette ADF démo complète pour Amiga 500
	@echo "💾 Création disquette ADF démo pour Amiga 500..."
	@mkdir -p $(BUILD_DIR)/disk_content/c
	@mkdir -p $(BUILD_DIR)/disk_content/s
	@cp $(DISK_DEMO)/c/* $(BUILD_DIR)/disk_content/c/
	@cp $(DISK_DEMO)/s/* $(BUILD_DIR)/disk_content/s/
	@cp $(TARGET_DEMO) $(BUILD_DIR)/disk_content/
	@cp $(ASSETS_DIR)/*.mod $(BUILD_DIR)/disk_content/ 2>/dev/null || true
	./create-intro.sh $(BUILD_DIR)/disk_content $(BUILD_DIR)/demo_amiga.adf 'A500-DEMO'
	@echo "✅ Disquette ADF démo créée: $(BUILD_DIR)/demo_amiga.adf"
	@echo "🚀 Compatible Amiga 500 + Kickstart 1.3"
	@rm -rf $(BUILD_DIR)/disk_content

adf: adf-simple ## Alias pour créer la disquette simple par défaut

list: ## Liste les binaires et images disponibles
	@echo "📦 Fichiers Amiga compilés:"
	@if [ -d $(BUILD_DIR) ]; then \
		ls -lah $(BUILD_DIR)/; \
	else \
		echo "  Aucun fichier trouvé. Utilisez 'make simple' ou 'make adf'."; \
	fi

convert-png: ## Convertit une image PNG (Usage: make convert-png INPUT=source.png OUTPUT=dest.png COLORS=256)
	@if [ -z "$(INPUT)" ] || [ -z "$(OUTPUT)" ]; then \
		echo "❌ Usage: make convert-png INPUT=source.png OUTPUT=dest.png [COLORS=256]"; \
		echo "   Exemple: make convert-png INPUT=assets/big.png OUTPUT=assets/small.png COLORS=8"; \
		exit 1; \
	fi
	@COLORS_PARAM=$${COLORS:-256}; \
	echo "🖼️  Conversion PNG: $(INPUT) -> $(OUTPUT) ($$COLORS_PARAM couleurs)"; \
	convert "$(INPUT)" -background white -alpha remove -colors $$COLORS_PARAM "$(OUTPUT)"; \
	echo "✅ Conversion terminée"; \
	ls -lh "$(INPUT)" "$(OUTPUT)" || true