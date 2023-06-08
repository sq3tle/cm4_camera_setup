# Skrypty do płytki Kreweta CM4

Instrukcja pierwszego setupu:

    sudo apt-get update
    sudo apt-get install ansible git
    git clone https://github.com/sq3tle/cm4_camera_setup .
    ansible-playbook setup.yml
    
Po zakończeniu playbooka zrestartuj raspberry pi:

    sudo restart now
Po ponownym połączeniu, zsynchronizuj pierwszy raz zegar rtc:

     ansible-playbook sync_rtc.yml

Płytka jest gotowa do pracy. Oto najważniejsze komendy
| Polecenie | Opis |
|--|--|
| ansible-playbook enable.yml | Uzbraja i uruchamia rejestrację danych, nawet po resecie zasilania |
| ansible-playbook disable.yml | Wyłącza rejestrację danych, nawet po resecie zasilania  |
| ansible-playbook sync_rtc.yml | Synchronizuje zegar RTC z obecnego czasu systemowego   |
| ansible-playbook purge.yml | Usuwa dotychczasowe dane  |
