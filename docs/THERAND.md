# Therand TV — stratégie du fork Omega

## Branches

- `Omega` : référence upstream historique, aucune modification Therand ;
- `therand-omega` : branche d'intégration de la version Kodi 21/Omega ;
- branches `feature/*` : développement par pull request vers `therand-omega` ;
- `Piers` : branche upstream Kodi 22 conservée mais hors périmètre de la version actuelle.

## Base actuelle

La branche `Omega` du fork contient IPTV Simple `21.11.0` et dépend des familles `inputstream.*` 21.x. Elle est donc la base de la version Kodi 21/Omega de Therand TV.

## Objectif fonctionnel

Le fork conserve volontairement l'identifiant `pvr.iptvsimple` et remplace directement IPTV Simple Client sur le Kodi cible. Ce choix permet de réutiliser les données d'instance et les réglages IPTV Simple existants au lieu de créer un second client PVR à reconfigurer.

Toutes les fonctions IPTV Simple utiles restent présentes :

- M3U et XMLTV ;
- groupes, logos et fournisseurs ;
- lecture HTTP/HLS/DASH ;
- catch-up/timeshift existant ;
- médias/VOD déjà exposés par IPTV Simple dans les enregistrements.

Le fork ajoute le backend d'enregistrement Therand :

- timers Kodi natifs ;
- enregistrement immédiat ;
- enregistrement depuis l'EPG ;
- timer manuel sans EPG ;
- titre du programme EPG utilisé pour nommer l'enregistrement lorsqu'il est disponible ;
- absence d'EPG sur un enregistrement instantané = saisie obligatoire de la durée ;
- catalogue des enregistrements terminés dans `TV > Enregistrements` ;
- lecture locale du fichier `.ts` sur LibreELEC ;
- suppression native depuis Kodi ;
- rafraîchissement automatique des timers/enregistrements lorsque le scheduler VPS change d'état ;
- communication avec `therand-tv-recorder` sur le VPS via WireGuard.

La vidéo ne transite pas par le VPS. Le backend VPS orchestre un agent LibreELEC qui exécute FFmpeg en copie directe et écrit directement sur le disque local.

## Configuration recorder MVP

La configuration spécifique Therand est volontairement séparée des réglages upstream pendant les premiers tests. Elle se trouve dans le dossier userdata de `pvr.iptvsimple`, dans `therand-recorder.xml`.

Voir `docs/therand-recorder.xml.example`.

Champs :

- `enabled` : active les capacités timers/recordings Therand ;
- `backend_url` : URL privée WireGuard du backend VPS, actuellement `http://10.13.13.1:8787` ;
- `token` : jeton Bearer backend, jamais versionné ;
- `recordings_root` : chemin hôte LibreELEC du dossier contenant les `.ts` ;
- `margin_before_seconds` / `margin_after_seconds` : marges par défaut.

Le backend ne renvoie que des chemins relatifs. Le client refuse les chemins contenant une traversée `..` avant de les joindre à `recordings_root`.

## Mise à jour de l'addon sur Kodi

La distribution Therand conserve l'identifiant `pvr.iptvsimple` afin qu'une installation par-dessus l'addon officiel garde les réglages existants.

Le premier release candidate complet est empaqueté sous la version de distribution `21.99.0+therand.3` avec le nom visible `IPTV Simple Client (Therand DVR)`. Cette version reste dans la famille Kodi 21/Omega tout en étant supérieure aux versions Omega upstream actuelles, ce qui évite qu'une mise à jour 21.x officielle remplace silencieusement la variante Therand.

Les nouveautés upstream sont intégrées via le workflow de synchronisation ci-dessous, puis testées dans notre fork avant de produire une nouvelle révision Therand.

Le job GCC publie également le ZIP compilé comme artifact GitHub Actions `pvr.iptvsimple-omega-therand`.

## Synchronisation upstream

`.github/workflows/sync-upstream-omega.yml` vérifie périodiquement la branche `Omega` de `kodi-pvr/pvr.iptvsimple`.

Lorsqu'une nouveauté est détectée :

1. une branche `sync/upstream-omega` est reconstruite depuis `therand-omega` ;
2. la nouvelle branche upstream Omega est fusionnée ;
3. une PR est ouverte vers `therand-omega` ;
4. le workflow de build normal compile alors le résultat avec GCC et Clang avant tout merge.

Aucun changement upstream n'est fusionné automatiquement dans `therand-omega`.

## Règle de migration Kodi

Kodi 22/Piers fera l'objet d'une branche Therand distincte. Nous ne mélangerons jamais les changements d'API PVR Piers dans la branche Omega.
