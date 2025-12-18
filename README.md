# Projet_ADS_1220
Prototype IoT pour monitorer en temps réel les performances thermiques des bâtiments.  Mesure de variations de tension avec ADC ADS1220, intégration réseau et connexion à ThingsBoard pour stockage et visualisation des métriques.

🛠️ Matériel -ADC : ADS1220

-Microcontrôleurs : ESP32

-Capteurs thermiques : prévus dans la configuration finale, mais remplacés pour le moment par un potentiomètre afin de simuler les variations de tension

💻 Logiciel :

Langages : C/C++,

Bibliothèques :Protocentral_ADS1220, PubSubClient, ArduinoJson

Plateforme IoT : ThingsBoard (RPC, télémétrie, Dashboard)

🔧 Environnement de développement:
Le projet est développé avec PlatformIO, un environnement moderne et extensible pour l’embarqué.

🔗 Intégration réseau : 
connexion wifi

Connexion via MQTT à ThingsBoard

Gestion des RPC pour reconfigurer à distance les paramètres des ADC (PGA, Data Rate, etc.)

Visualisation des métriques sur Dashboard (température, Tension)

📊 Résultats : Prototype fonctionnel capable de mesurer et transmettre des variations de tension.

Dashboard interactif pour le suivi en temps réel des performances thermiques des bâtiments.
