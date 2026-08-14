# IPTV Simple Client (Therand DVR) — Omega RC1 (historique)

> **Document historique.** Cette RC1 (`21.99.0+therand.3`) n'est plus la version déployée. L'état de référence actuel est documenté dans `docs/THERAND.md` et correspond à `21.99.0+therand.5` sur `wg-vps` `10.13.14.x`.

Distribution Kodi 21/Omega historique : `21.99.0+therand.3`.

## Fonctions ajoutées à cette étape

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

## Architecture de cette RC1

Cette RC1 documentait encore l'ancien réseau recorder `10.13.13.x`. Cette architecture a depuis été remplacée par le tunnel privé `wg-vps` `10.13.14.x` :

- backend recorder VPS actuel : `http://10.13.14.1:8787` ;
- agent recorder LibreELEC actuel : `http://10.13.14.2:8788` ;
- proxy VAVOO LibreELEC : `http://127.0.0.1:8899` ;
- stockage : `/var/media/nvme0n1p7-nvme-Samsung_SSD_990/TV-Recordings`.

Le VPS ne stocke aucun fichier vidéo. FFmpeg s'exécute sur LibreELEC en copie directe (`-c copy`).

Pour l'état réellement déployé et validé, utiliser `docs/THERAND.md`.
