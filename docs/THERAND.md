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
- catch-up/timeshift existant.

Le fork ajoute un backend d'enregistrement distant :

- timers Kodi natifs ;
- enregistrement immédiat ;
- enregistrement depuis l'EPG ;
- timer manuel sans EPG ;
- titre du programme EPG utilisé pour nommer l'enregistrement lorsqu'il est disponible ;
- catalogue des enregistrements ;
- suppression des enregistrements ;
- communication avec `therand-tv-recorder` sur le VPS via WireGuard.

La vidéo ne transitera pas par le VPS. Le backend VPS orchestre un agent LibreELEC qui exécute FFmpeg et écrit directement sur le disque local.

## Mise à jour de l'addon sur Kodi

La distribution Therand doit conserver l'identifiant `pvr.iptvsimple` afin qu'une installation par-dessus l'addon officiel garde les réglages existants. La version de distribution sera distinguée de l'upstream au moment de produire les ZIP afin d'éviter qu'une mise à jour officielle remplace silencieusement notre variante. Les nouveautés upstream sont intégrées via le workflow de synchronisation ci-dessous, pas directement sur le Kodi de production.

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
