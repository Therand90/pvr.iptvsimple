# Therand TV — stratégie du fork Omega

## Branches

- `Omega` : référence upstream historique, aucune modification Therand ;
- `therand-omega` : branche d'intégration de la version Kodi 21/Omega ;
- branches `feature/*` : développement par pull request vers `therand-omega` ;
- `Piers` : branche upstream Kodi 22 conservée mais hors périmètre de la version actuelle.

## Base actuelle

La branche `Omega` du fork contient IPTV Simple `21.11.0` et dépend des familles `inputstream.*` 21.x. Elle est donc la base de la version Kodi 21/Omega de Therand TV.

## Objectif fonctionnel

Le fork deviendra `pvr.therandtv` et conservera toutes les fonctions IPTV Simple utiles :

- M3U et XMLTV ;
- groupes, logos et fournisseurs ;
- lecture HTTP/HLS/DASH ;
- catch-up/timeshift existant.

Il ajoutera un backend d'enregistrement distant :

- timers Kodi natifs ;
- enregistrement immédiat ;
- enregistrement depuis l'EPG ;
- timer manuel sans EPG ;
- catalogue des enregistrements ;
- suppression des enregistrements ;
- communication avec `therand-tv-recorder` sur le VPS via WireGuard.

La vidéo ne transitera pas par le VPS. Le backend VPS orchestre un agent LibreELEC qui exécute FFmpeg et écrit directement sur le disque local.

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
