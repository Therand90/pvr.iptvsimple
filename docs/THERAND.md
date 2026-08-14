# Therand TV — fork IPTV Simple Omega

## Branches

- `Omega` : référence upstream historique, sans modifications Therand ;
- `therand-omega` : branche d'intégration Therand pour Kodi 21/Omega ;
- branches `feature/*` / `fix/*` / `docs/*` : travail par pull request vers `therand-omega` ;
- `Piers` : branche upstream Kodi 22, hors périmètre de la version actuellement déployée.

## Objectif

Le fork conserve volontairement l'identifiant `pvr.iptvsimple` et remplace directement IPTV Simple Client sur le Kodi cible. Cela permet de réutiliser les instances, M3U/XMLTV et réglages existants au lieu d'installer un second client PVR.

Toutes les fonctions IPTV Simple utiles restent présentes. Le fork ajoute le système d'enregistrement Therand :

- timers Kodi natifs ;
- enregistrement immédiat ;
- enregistrement depuis l'EPG ;
- timer manuel sans EPG ;
- titre du programme EPG utilisé pour nommer l'enregistrement lorsqu'il est disponible ;
- saisie de durée lorsqu'aucun EPG exploitable n'est disponible ;
- catalogue des enregistrements terminés dans `TV > Enregistrements` ;
- lecture locale du fichier `.ts` sur LibreELEC ;
- suppression native depuis Kodi ;
- rafraîchissement automatique des timers/enregistrements lorsque le scheduler VPS change d'état ;
- communication avec `therand-tv-recorder` sur le VPS via WireGuard privé.

La vidéo ne transite jamais par le VPS. Le backend orchestre un agent LibreELEC qui exécute FFmpeg en copie directe et écrit sur le disque local.

## Version déployée validée

Version Kodi 21/Omega réellement installée et validée au 14 août 2026 :

```text
21.99.0+therand.5
```

Nom visible :

```text
IPTV Simple Client (Therand DVR)
```

Commit `therand-omega` correspondant :

```text
eb79a32d8c2fc4afc7b0987104a6a31319e3bde3
```

Archive de référence conservée sur LibreELEC :

```text
pvr.iptvsimple-21.99.0+therand.5-Omega-x86_64.zip
```

Le binaire installé `pvr.iptvsimple.so.21.99.0` est identique au binaire contenu dans cette archive, SHA-256 :

```text
39346616a10c158089ec31cd33ce575007c4e139e402916e41a7845ffd0011a1
```

## Sources live actuelles

Catch-up TV & More n'est plus utilisé comme source de direct. Les chaînes live de Therand TV ne reposent plus sur des cibles `plugin://`.

Le recorder reçoit des sources HTTP(S) enregistrables. Pour VAVOO, il utilise une URL logique stable du proxy local.

La révision `.5` conserve néanmoins la prise en charge du paramètre `recording_url` embarqué par Playlist Manager :

- si l'URL de lecture est déjà HTTP(S), elle est utilisée directement ;
- sinon, le client peut extraire `recording_url` et ne transmet au backend qu'une URL HTTP(S) valide ;
- aucune URL `plugin://` n'est envoyée à l'agent FFmpeg.

Cette compatibilité reste utile pour l'évolution du système, mais l'architecture live actuelle n'a plus besoin de Catch-up TV & More pour ses directs.

## Réseau recorder

Le recorder utilise exclusivement `wg-vps` :

- backend VPS : `10.13.14.1:8787` ;
- LibreELEC : `10.13.14.2` ;
- agent recorder : `10.13.14.2:8788`.

Le réseau `10.13.13.x` / `wg0` reste réservé au tunnel FR et au routage replay FR↔BE. Le client recorder ne modifie aucune route ni configuration WireGuard.

## Configuration recorder

La configuration spécifique Therand reste séparée des réglages upstream :

```text
/storage/.kodi/userdata/addon_data/pvr.iptvsimple/therand-recorder.xml
```

Voir `docs/therand-recorder.xml.example`.

Champs principaux :

- `enabled` : active les capacités timers/recordings Therand ;
- `backend_url` : actuellement `http://10.13.14.1:8787` ;
- `token` : jeton Bearer privé, jamais versionné ;
- `recordings_root` : dossier local contenant les `.ts` ;
- `margin_before_seconds` / `margin_after_seconds` : marges par défaut.

Le backend ne renvoie que des chemins relatifs et le client refuse les traversées `..` avant de construire le chemin local.

## Tests réels validés

Sur l'installation LibreELEC de référence :

- enregistrement direct : OK ;
- programmation : OK ;
- programmation depuis le guide TV : OK ;
- enregistrements terminés visibles dans Kodi : OK ;
- suppression depuis Kodi : OK.

## Packaging

La CI applique les métadonnées Therand au packaging et produit l'artifact :

```text
pvr.iptvsimple-omega-therand
```

Le build correspondant à `eb79a32d...` a réussi avec GCC et Clang.

## Synchronisation upstream

`.github/workflows/sync-upstream-omega.yml` surveille la branche `Omega` de `kodi-pvr/pvr.iptvsimple`.

Lorsqu'une nouveauté est détectée :

1. une branche de synchronisation est créée depuis `therand-omega` ;
2. la nouveauté upstream Omega y est intégrée ;
3. une PR est ouverte vers `therand-omega` ;
4. le résultat est compilé et testé avant toute fusion.

Aucun changement upstream n'est fusionné automatiquement.

Kodi 22/Piers fera l'objet d'une branche Therand distincte afin de ne pas mélanger les changements d'API PVR avec Omega.
