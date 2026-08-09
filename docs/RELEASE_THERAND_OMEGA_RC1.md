# IPTV Simple Client (Therand DVR) — Omega RC1

Distribution Kodi 21/Omega : `21.99.0+therand.3`.

## Fonctions ajoutées

- remplacement direct de `pvr.iptvsimple` afin de conserver les instances M3U/XMLTV existantes ;
- bouton d'enregistrement et timers PVR natifs pilotés par `therand-tv-recorder` ;
- enregistrement instantané jusqu'à la fin du programme EPG courant ;
- titre EPG utilisé pour le nom du fichier ;
- saisie de durée lorsqu'aucun EPG live n'est disponible ;
- programmation, modification et annulation via l'interface PVR Kodi ;
- remontée automatique des changements d'état du scheduler VPS ;
- enregistrements terminés visibles dans `TV > Enregistrements` ;
- lecture directe depuis le disque LibreELEC ;
- suppression depuis Kodi ;
- arrêt manuel d'un enregistrement conservant la partie déjà écrite.

## Architecture attendue

- backend recorder VPS : `http://10.13.13.1:8787`, dans le namespace réseau du conteneur WireGuard ;
- agent recorder LibreELEC : `http://10.13.13.2:8788`, en réseau Docker host ;
- proxy Vavoo LibreELEC : `http://127.0.0.1:8899` ;
- stockage : `/var/media/nvme0n1p7-nvme-Samsung_SSD_990/TV-Recordings`.

Le VPS ne stocke aucun fichier vidéo. FFmpeg s'exécute sur LibreELEC en copie directe (`-c copy`).

## Avant installation

Sauvegarder le dossier userdata de l'IPTV Simple actuel et déployer/valider d'abord le backend VPS et l'agent LibreELEC. Le fichier `therand-recorder.xml` contient le token privé du backend et ne doit jamais être versionné.
