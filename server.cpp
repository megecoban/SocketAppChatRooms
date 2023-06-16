#include <pthread.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>

using namespace std;

int SunucuPortu = 7778;
int maksKullaniciSayisi = 2;
int aktifKullaniciSayisi = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

class Kullanici
{
    public:
        int ID;
        int aktifOdaID = 0;
        string isim;
};

class KonusmaOdasi
{
    public:
        int odaID;
        Kullanici odaSahibi;
        string isim;
};

class Sunucu 
{
    public:
        int sunucuPort = SunucuPortu;
        int sunucuSoketi;
        int bufferBoyutu = 1024;
        struct sockaddr_in sunucuAddr;
        socklen_t sunucuAddrLen = sizeof(sunucuAddr);
};

Sunucu sunucu;

vector<Kullanici> aktifKullaniciListesi;

vector<KonusmaOdasi> aktifOdaListesi;

vector<Kullanici> misafirListesi;

string komutlar[8] = {"login","logout","list","enter","whereami","open","close","help"};

pair<string, string> KomutAyir(string girdi);


void ClientlaraYolla(Kullanici kullanici, string mesaj);
void ClientaYolla(Kullanici kullanici, string mesaj);
void OdaDuyurusuYolla(Kullanici kullanici, string mesaj);
void SunucuDuyurusu(string mesaj);
bool BuKullaniciIsmiVarMi(string kullaniciIsmi);
bool KullaniciEkle(Kullanici kullanici, string isim);
void KullaniciSil(Kullanici kullanici);
string OdadakiKullanicilariListele(Kullanici kullanici);
string KullaniciListele();
Kullanici* AktifKullaniciGetir(int ID);
bool BuOdaIsmiVarMi(string odaIsmi);
int OdaIDOlustur();
bool OdaEkle(Kullanici odaSahibi, string odaIsmi);
bool OdaEkle(Kullanici odaSahibi, int odaID, string odaIsmi);
KonusmaOdasi* OdaGetir(string odaIsmi);
void OdaSil(Kullanici kullanici, string odaIsmi);
void OdalariSil(Kullanici kullanici);
string OdaListele();
string HangiOda(int odaID);
void LobiOlustur();
void MesajBastir(Kullanici kullanici, string girdi);
void _OdayaGir(Kullanici kullanici, int roomIndex);
void OdayaGir(Kullanici kullanici, int odaID);
void OdayaIsimleGir(Kullanici kullanici, string odaIsmi);
bool KomutKontrol(string girdi);
bool BuKullaniciAktifMi(int ID);
pair<string, string> KomutAyir(string girdi);
void KullaniciGuncelle(int kullaniciID, string yeniIsim);
void KullaniciGuncelle(int kullaniciID, int yeniOdaID);
void KullaniciGuncelle(int kullaniciID, string yeniIsim, int yeniOdaID);
void KomutYurut(Kullanici kullanici, string girdi);
void GirdiAl(Kullanici kullanici, string girdi);
void SocketBaslat();
void SunucuBaslat();
void OdaDuyurusuYolla(Kullanici kullanici, string mesaj);
void ClientlaraYolla(Kullanici kullanici, string mesaj);
void ClientaYolla(Kullanici kullanici, string mesaj);


// KULLANICI ISLEMLERI

bool BuKullaniciIsmiVarMi(string kullaniciIsmi){
    bool kontrol = false;
    for(int i=0; i<aktifKullaniciListesi.size(); i++)
    {
        if(aktifKullaniciListesi[i].isim == kullaniciIsmi)
        {
            kontrol = true;
            break;
        }
    }
    return kontrol;
}

bool KullaniciEkle(Kullanici kullanici, string isim){
    if(BuKullaniciIsmiVarMi(isim))
    {
        ClientaYolla(kullanici, "Bu kullanıcı ismi kullanılıyor. Lütfen başka deneyin. ["+ isim +"]");
        return false;
    }

    Kullanici yeniKullanici;
    yeniKullanici.ID = kullanici.ID;
    yeniKullanici.isim = isim;
    yeniKullanici.aktifOdaID = 0; // LOBI
    aktifKullaniciListesi.push_back(yeniKullanici);
    return true;
}

void KullaniciSil(Kullanici kullanici)
{
    bool kontrol = false;
    for(int i=0; i<aktifKullaniciListesi.size(); i++)
    {
        if(kullanici.ID == aktifKullaniciListesi[i].ID)
        {
            aktifKullaniciListesi.erase(aktifKullaniciListesi.begin() + i);
            kontrol = true;
            break;
        }
    }
}

string OdadakiKullanicilariListele(Kullanici kullanici){
    string returnVal = "Odadaki aktif kullanicilar:\n";
    
    int odaID = AktifKullaniciGetir(kullanici.ID)->aktifOdaID;
        for(int i=0; i<aktifKullaniciListesi.size(); i++)
        {
            if(aktifKullaniciListesi[i].aktifOdaID == odaID)
                returnVal+=to_string(i)+" , Isim: "+aktifKullaniciListesi[i].isim+"\n";
        }
    return returnVal;
}

string KullaniciListele(){
    string returnVal = "";
    if(aktifKullaniciListesi.size() == 0)
    {
        returnVal = "\nAktif Kullanici Sayisi: 0\nAktif kullanici yok.\n";
    }else{
        returnVal = "\nAktif Kullanici Sayisi: "+to_string(aktifKullaniciListesi.size())+"\n";
        for(int i=0; i<aktifKullaniciListesi.size(); i++)
        {
            returnVal+=to_string(i)+" , Isim: "+aktifKullaniciListesi[i].isim+"\n";
        }
    }
    return returnVal;
}

Kullanici* AktifKullaniciGetir(int ID)
{
    for(int i=0; i<aktifKullaniciListesi.size(); i++)
        if(aktifKullaniciListesi[i].ID == ID)
            return &(aktifKullaniciListesi[i]);

    return nullptr;
}

// ODA ISLEMLERI

bool BuOdaIsmiVarMi(string odaIsmi){
    bool kontrol = false;
    for(int i=0; i<aktifOdaListesi.size(); i++)
        if(aktifOdaListesi[i].isim == odaIsmi)
        {
            kontrol = true;
            break;
        }
        
    return kontrol;
}

int OdaIDOlustur(){
    int odaID = 0;
    for(int i=0; i<aktifOdaListesi.size(); i++)
    {
        if(aktifOdaListesi[i].odaID >= odaID)
        {
            odaID = aktifOdaListesi[i].odaID + 1;
        }
    }

    return odaID;
}

bool OdaEkle(Kullanici odaSahibi, string odaIsmi){ 
    if(BuOdaIsmiVarMi(odaIsmi))
    {
        string mesaj = "Bu oda mevcut. Lutfen baska isimle deneyin. ["+ odaIsmi +"]";
        ClientaYolla(odaSahibi, mesaj);
        return false;
    }

    KonusmaOdasi yeniOda;
    
    yeniOda.odaSahibi = odaSahibi;
    yeniOda.odaID = OdaIDOlustur();
    yeniOda.isim = odaIsmi;
    aktifOdaListesi.push_back(yeniOda);
    string mesaj = "Oda eklendi: "+odaIsmi;
    ClientaYolla(odaSahibi, mesaj);
    return true;
}

bool OdaEkle(Kullanici odaSahibi, int odaID, string odaIsmi){ 
    if(BuOdaIsmiVarMi(odaIsmi))
    {
        ClientaYolla(odaSahibi, "Bu oda mevcut. Lutfen baska isimle deneyin. ["+ odaIsmi +"]");
        return false;
    }

    KonusmaOdasi yeniOda;

    yeniOda.odaSahibi = odaSahibi;
    yeniOda.odaID = odaID;
    yeniOda.isim = odaIsmi;
    aktifOdaListesi.push_back(yeniOda);
    ClientaYolla(odaSahibi, "Oda eklendi; "+odaIsmi);
    return true;
}

KonusmaOdasi* OdaGetir(string odaIsmi)
{
    for(int i=0; i<aktifOdaListesi.size(); i++){
        if(aktifOdaListesi[i].isim == odaIsmi)
        {
            return &aktifOdaListesi[i];
        }
    }

    return nullptr;
}

void OdaSil(Kullanici kullanici, string odaIsmi)
{
    KonusmaOdasi* temp = OdaGetir(odaIsmi);
    KonusmaOdasi oda = *temp;

    Kullanici kullaniciTemp;

    for(int i=0; i<aktifKullaniciListesi.size(); i++)
        if(aktifKullaniciListesi[i].ID == kullanici.ID)
            kullaniciTemp = aktifKullaniciListesi[i];

    if(oda.odaSahibi.ID != kullaniciTemp.ID)
    {
        ClientaYolla(kullanici, "Bu oda size ait degil.");
        return;
    }

    if(odaIsmi == "Lobi")
    {
        ClientaYolla(kullanici, "Lobi silinemez.");
        return;
    }

    bool kontrol = false;
    int tempOdaID = -1;
    for(int i=0; i<aktifOdaListesi.size(); i++)
    {
        if(odaIsmi == aktifOdaListesi[i].isim)
        {
            tempOdaID = aktifOdaListesi[i].odaID;
            aktifOdaListesi.erase(aktifOdaListesi.begin() + i);
            kontrol = true;
            break;
        }
    }

    if(kontrol)
    {
        for(int i=0; i<aktifKullaniciListesi.size(); i++)
        {
            if(aktifKullaniciListesi[i].ID == kullanici.ID || aktifKullaniciListesi[i].aktifOdaID == tempOdaID)
            {
                SunucuDuyurusu(aktifOdaListesi[i].isim + " odasi kapatilmistir. Odadakiler Lobi'ye yonlendirildi.");
                aktifKullaniciListesi[i].aktifOdaID = 0; // Lobiye esitlendi
            }
        }
    }
}

void OdalariSil(Kullanici kullanici){
    for(int i=aktifOdaListesi.size()-1; i>-1; i--)
    {
        if(kullanici.ID == aktifOdaListesi[i].odaSahibi.ID)
        {
            SunucuDuyurusu(aktifOdaListesi[i].isim + " odasi kapatilmistir. Odadakiler Lobi'ye yonlendirildi.");

            int tempOdaID = aktifOdaListesi[i].odaID;

            for(int i=0; i<aktifKullaniciListesi.size(); i++)
            {
                if(aktifKullaniciListesi[i].aktifOdaID == tempOdaID)
                    aktifKullaniciListesi[i].aktifOdaID = 0; // Lobiye esitlendi
            }

            aktifOdaListesi.erase(aktifOdaListesi.begin() + i);
            break;
        }
    }
}

string OdaListele(){
    string returnVal = "\nAktif Oda Sayisi: "+to_string(aktifOdaListesi.size())+"\n";
    for(int i=0; i<aktifOdaListesi.size(); i++)
        returnVal +=to_string(i)+" , Isim: "+aktifOdaListesi[i].isim+"\n";

    return returnVal;
}

string HangiOda(int odaID)
{
    string returnVal = "";
    returnVal = "Lobi";
    for(int i=0; i<aktifOdaListesi.size(); i++)
        if(aktifOdaListesi[i].odaID == odaID)
        {
            returnVal = aktifOdaListesi[i].isim;
            break;
        }

    return returnVal;
}

void LobiOlustur(){
    Kullanici lobiSahibi;
    lobiSahibi.aktifOdaID = -1;
    lobiSahibi.ID = -1;
    lobiSahibi.isim = "LobiSahibi";
    OdaEkle(lobiSahibi, 0, "Lobi");
}

// KONTROLLER

void MesajBastir(Kullanici kullanici, string girdi)
{
    ClientlaraYolla(kullanici, girdi);
}

void _OdayaGir(Kullanici kullanici, int roomIndex)
{

    if(AktifKullaniciGetir(kullanici.ID) != nullptr)
    {
        Kullanici* temp = AktifKullaniciGetir(kullanici.ID);
        temp->aktifOdaID = aktifOdaListesi[roomIndex].odaID;
        cout << "aktifodalistesi roomIndex odaID = " << aktifOdaListesi[roomIndex].odaID << endl;
    }else{
        for(int i=0; i<aktifKullaniciListesi.size(); i++)
            if(aktifKullaniciListesi[i].ID == kullanici.ID){
                aktifKullaniciListesi[i].aktifOdaID = aktifOdaListesi[roomIndex].odaID;
                return;
            }

        kullanici.aktifOdaID = aktifOdaListesi[roomIndex].odaID;
    }

    string mesaj = aktifOdaListesi[roomIndex].isim + " odasina hos geldiniz.";
    ClientaYolla(kullanici, mesaj);
}

void OdayaGir(Kullanici kullanici, int odaID)
{
    Kullanici* temp = AktifKullaniciGetir(kullanici.ID);
    temp->aktifOdaID = odaID;
}

void OdayaIsimleGir(Kullanici kullanici, string odaIsmi)
{
    Kullanici* temp = AktifKullaniciGetir(kullanici.ID);
    KonusmaOdasi* tempOda = OdaGetir(odaIsmi);

    temp->aktifOdaID = tempOda->odaID;
}

bool KomutKontrol(string girdi)
{
    string komut = KomutAyir(girdi).first;

    for(int i=0; i<sizeof(komutlar)/sizeof(komutlar[0]); i++)
    {
        if(komut == komutlar[i])
        {
            return true;
        }
    }
    return false;
}

bool BuKullaniciAktifMi(int ID)
{
    for(int i=0; i<aktifKullaniciListesi.size(); i++)
    {
        if(aktifKullaniciListesi[i].ID == ID && aktifKullaniciListesi[i].isim.empty() == false)
            return true;
    }
    return false;
}

pair<string, string> KomutAyir(string girdi)
{
    size_t boslukKarakterininIndexi = girdi.find(' ');
    string komut = "";
    string parametre = "";
    if(boslukKarakterininIndexi != string::npos)
    {
        komut = girdi.substr(0, boslukKarakterininIndexi);
        parametre = girdi.substr(boslukKarakterininIndexi+1);
    }else{
        komut = girdi;
        parametre = "";
    }

    return make_pair(komut, parametre);
}

void KullaniciGuncelle(int kullaniciID, string yeniIsim)
{
    if(AktifKullaniciGetir(kullaniciID) != nullptr)
    {
        AktifKullaniciGetir(kullaniciID)->isim = yeniIsim;
    }
}

void KullaniciGuncelle(int kullaniciID, int yeniOdaID)
{
    if(AktifKullaniciGetir(kullaniciID) != nullptr)
    {
        AktifKullaniciGetir(kullaniciID)->aktifOdaID = yeniOdaID;
    }
}

void KullaniciGuncelle(int kullaniciID, string yeniIsim, int yeniOdaID)
{
    if(AktifKullaniciGetir(kullaniciID) != nullptr)
    {
        AktifKullaniciGetir(kullaniciID)->isim = yeniIsim;
        AktifKullaniciGetir(kullaniciID)->aktifOdaID = yeniOdaID;
    }
}

void KomutYurut(Kullanici kullanici, string girdi){
    
    string komut = KomutAyir(girdi).first;
    string parametre = KomutAyir(girdi).second;

    bool parametreVarMi = parametre.empty() ? false : true;

    if(parametreVarMi)
    {
        if(komut=="login")
        {
            for(int i=0; i<aktifKullaniciListesi.size(); i++)
                if(aktifKullaniciListesi[i].ID == kullanici.ID)
                    return;

            KullaniciEkle(kullanici, parametre);

            for(int i=0; i<aktifKullaniciListesi.size(); i++)
                if(aktifKullaniciListesi[i].ID == kullanici.ID)
                    OdaDuyurusuYolla(kullanici, aktifKullaniciListesi[i].isim+" sunucuya katildi.");

            return;
        }else if(komut=="list")
        {
            if(parametre == "rooms")
            {
                ClientaYolla(kullanici, OdaListele());
            }else if(parametre == "users")
            {
                ClientaYolla(kullanici, OdadakiKullanicilariListele(kullanici));
            }
        }else if(komut=="enter")
        {
            if(BuOdaIsmiVarMi(parametre))
            {
                OdayaIsimleGir(kullanici, parametre);
                if(AktifKullaniciGetir(kullanici.ID) != nullptr)
                {
                    OdaDuyurusuYolla(kullanici, AktifKullaniciGetir(kullanici.ID)->isim+" odaya girdi.");
                }else{
                    OdaDuyurusuYolla(kullanici, kullanici.isim+" odaya girdi.");
                }
            }else{
                ClientaYolla(kullanici, "Bu oda mevcut degil.");
            }
        }else if(komut=="open")
        {
            if(BuOdaIsmiVarMi(parametre) == false)
            {
                OdaEkle(kullanici, parametre);
            }else{
                string mesaj = "Bu oda ismi var. Lütfen başka isim seçin.";
                ClientaYolla(kullanici, mesaj);
            }
        }else if(komut=="close")
        {
            if(BuOdaIsmiVarMi(parametre))
            {
                OdaSil(kullanici, parametre);
            }else{
                string mesaj = "Bu isimde bir oda yok.";
                ClientaYolla(kullanici, mesaj);
            }
        }
    }else{
        if(komut=="help")
        {
            /*
            o========== help ==========o \n
            | login <nickname> : kullanıcıyı IRC sunucusunu verilen mahlas ile kaydeder.\n
            | logout : kullanıcının sunucudaki oturumunu kapatır.\n
            | list rooms : sunucudaki mevcut sohbet odalarını listeler.\n
            | list users : geçerli sohbet odasındaki kullanıcıları listeler.\n
            | enter <room_name> : misafirin bir sohbet odasına girişini (bağlanmasını) sağlar.\n
            | whereami : misafirin içinde bulunduğunuz sohbet odasının adını yazar.\n
            | open <room_name> : yeni bir sohbet odası oluşturur.\n
            | close <room_name> : belirtilen sohbet odasını kapatır. [Sadece odayı oluşturan kişi kullanabilir.]\n
            | help : komutları ekrana bastırır.\n
            o========================o
            */
            string mesaj = "o========== help ==========o \n| login <nickname> : kullanıcıyı IRC sunucusunu verilen mahlas ile kaydeder.\n| logout : kullanıcının sunucudaki oturumunu kapatır.\n| list rooms : sunucudaki mevcut sohbet odalarını listeler.\n| list users : geçerli sohbet odasındaki kullanıcıları listeler.\n| enter <room_name> : misafirin bir sohbet odasına girişini (bağlanmasını) sağlar.\n| whereami : misafirin içinde bulunduğunuz sohbet odasının adını yazar.\n| open <room_name> : yeni bir sohbet odası oluşturur.\n| close <room_name> : belirtilen sohbet odasını kapatır. [Sadece odayı oluşturan kişi kullanabilir.]\n| help : komutları ekrana bastırır.\no========================o";
            ClientaYolla(kullanici, mesaj);
        
        }else if(komut=="logout")
        {
            if(BuKullaniciAktifMi(kullanici.ID))
            {
                OdalariSil(kullanici);
                string temp = AktifKullaniciGetir(kullanici.ID)->isim;
                OdaDuyurusuYolla(kullanici, temp+" sunucudan ayrildi.");
                KullaniciSil(kullanici);
                kullanici.isim = "";
            }
        }else if(komut=="whereami")
        {
            string mesaj;

            if(AktifKullaniciGetir(kullanici.ID) != nullptr)
            {
                mesaj = to_string(AktifKullaniciGetir(kullanici.ID)->aktifOdaID) + " IDli " + HangiOda(AktifKullaniciGetir(kullanici.ID)->aktifOdaID) + " odasindasiniz.";
            }else{
                mesaj = to_string(kullanici.aktifOdaID) + " IDli " + HangiOda(kullanici.aktifOdaID) + " odasindasiniz.";
            }

            ClientaYolla(kullanici, mesaj);
        }
    }
}

void GirdiAl(Kullanici kullanici, string girdi)
{
    if(BuKullaniciAktifMi(kullanici.ID))
    {
        if(KomutKontrol(girdi))
        {
            KomutYurut(kullanici, girdi);
        }else{
            MesajBastir(kullanici, girdi);
        }
    }else{
        if(KomutKontrol(girdi) && KomutAyir(girdi).first == "login")
        {
            KomutYurut(kullanici, girdi);
        }else{
            ClientaYolla(kullanici, "Mesaj yollamadan once login olunuz.");
        }
    }
}


// SUNUCU ISLEMLERI


void SunucuBaslat(){
    SocketBaslat();
}

void* KullaniciBaglanti(void* args){

    pair<Kullanici*, int*>* gelenParametre = static_cast<pair<Kullanici*, int*>*>(args);

    Kullanici temp = *gelenParametre->first;
    int acceptSock = *gelenParametre->second;
    
    bool girisYaptiMi = false;

    char gelenMesaj[1024] = {0};

    while (true)
    {
        memset(gelenMesaj, 0, sizeof(gelenMesaj));
        int oku = read(acceptSock, gelenMesaj, 1024);
        if(oku > 0){
            GirdiAl(temp, gelenMesaj);
        }else if(oku<=0)
        {
            break;
        }
    }

    pthread_mutex_lock(&mutex);
    aktifKullaniciSayisi--;
    pthread_mutex_unlock(&mutex);

    Kullanici _temp;

    if(AktifKullaniciGetir(_temp.ID) != nullptr)
    {
        Kullanici* __temp = AktifKullaniciGetir(_temp.ID);
        _temp = *__temp;
    }else{
        _temp = temp;
    }

    string mesaj = ((_temp.isim == "" || _temp.isim.empty()) ? "Bir kisi" : _temp.isim)+ " sunucudan ayrildi.";
    OdaDuyurusuYolla(_temp, mesaj);
    return NULL;
}

void SocketBaslat(){
    sunucu.sunucuSoketi = socket(AF_INET, SOCK_STREAM, 0);

    if(sunucu.sunucuSoketi<0)
    {
        perror("\nSoket acilamadi.");
        exit(EXIT_FAILURE);
    }else{
        cout<<"\nSoket acildi."<<endl;
    }

    sunucu.sunucuAddr.sin_family = AF_INET;
    sunucu.sunucuAddr.sin_addr.s_addr = INADDR_ANY;
    sunucu.sunucuAddr.sin_port = htons(sunucu.sunucuPort);

    if(bind(sunucu.sunucuSoketi, (struct sockaddr*)&sunucu.sunucuAddr, sizeof(sunucu.sunucuAddr))<0)
    {
        perror("\nBind basarisiz.");
        exit(EXIT_FAILURE);
    }else{
        cout<<"\nBind basarili."<<endl;
    }

    if(listen(sunucu.sunucuSoketi, 16)<0)
    {
        perror("\nDinleme basarisiz.");
        exit(EXIT_FAILURE);
    }else{
        cout<<"\nDinleme basarili."<<endl;
    }

    int acceptSock;

    while ((acceptSock = accept(sunucu.sunucuSoketi, (struct sockaddr*)&sunucu.sunucuAddr, (socklen_t*)&sunucu.sunucuAddrLen))){
        if(aktifKullaniciSayisi>=maksKullaniciSayisi)
        {
            cout << "Sunucu dolu."<<endl;
            close(acceptSock);
            cout << "Isimsiz kisi sunucu kapisindan dondu."<<endl;
        }
        
        pthread_mutex_lock(&mutex);
        if(aktifKullaniciSayisi < maksKullaniciSayisi)
        {
            aktifKullaniciSayisi++;

            Kullanici yeniKullanici;
            yeniKullanici.ID = acceptSock;

            cout << "Isimsiz kisi lobiye katildi." << endl;

            pair<Kullanici*,int*> Parametre(&yeniKullanici, &acceptSock);

            pthread_t yeniBaglantiThreadi;
            pthread_create(&yeniBaglantiThreadi, NULL, KullaniciBaglanti, static_cast<void*>(&Parametre));
        }
        pthread_mutex_unlock(&mutex);
    }

    close(sunucu.sunucuSoketi);
    cout<<"Socket kapatildi."<<endl;
}


void SunucuDuyurusu(string mesaj)
{
    int socketNumarasi = 0;
    char msg[1024] = {0};

    string gonderilecekMesaj = mesaj;
    strcpy(msg, gonderilecekMesaj.c_str());

    for(int i=0; i<aktifKullaniciListesi.size(); i++)
        send(aktifKullaniciListesi[i].ID, msg, 1024, 0);

    cout << "LOG: " << gonderilecekMesaj << endl;
}

void OdaDuyurusuYolla(Kullanici kullanici, string mesaj)
{
    int socketNumarasi = 0;
    char msg[1024] = {0};

    string gonderilecekMesaj = mesaj;
    strcpy(msg, gonderilecekMesaj.c_str());

    for(int i=0; i<aktifKullaniciListesi.size(); i++)
        if(aktifKullaniciListesi[i].aktifOdaID == kullanici.aktifOdaID) 
            send(aktifKullaniciListesi[i].ID, msg, 1024, 0);

    cout << "LOG: " << gonderilecekMesaj << endl;
}

void ClientlaraYolla(Kullanici kullanici, string mesaj)
{
    int socketNumarasi = 0;
    char msg[1024] = {0};

    Kullanici sunucudakiKullanici;

    if(AktifKullaniciGetir(kullanici.ID) != nullptr)
    {
        Kullanici* temp = AktifKullaniciGetir(kullanici.ID);
        sunucudakiKullanici = *temp;
    }else{
        sunucudakiKullanici = kullanici;
    }

    string gonderilecekMesaj = "[ID: "+to_string(sunucudakiKullanici.ID)+" | OdaID: "+to_string(sunucudakiKullanici.aktifOdaID)+"] "+sunucudakiKullanici.isim+": "+mesaj+"\n";
    strcpy(msg, gonderilecekMesaj.c_str());

    for(int i=0; i<aktifKullaniciListesi.size(); i++)
        if(aktifKullaniciListesi[i].aktifOdaID == sunucudakiKullanici.aktifOdaID)
            send(aktifKullaniciListesi[i].ID, msg, 1024, 0);

    cout << "LOG: " << gonderilecekMesaj << endl;
}

void ClientaYolla(Kullanici kullanici, string mesaj)
{
    int socketNumarasi = 0;
    char msg[1024] = {0};

    Kullanici sunucudakiKullanici;

    if(AktifKullaniciGetir(kullanici.ID) != nullptr)
    {
        Kullanici* temp = AktifKullaniciGetir(kullanici.ID);
        sunucudakiKullanici = *temp;
    }else{
        sunucudakiKullanici = kullanici;
    }

    string gonderilecekMesaj = mesaj;
    strcpy(msg, gonderilecekMesaj.c_str());

    send(sunucudakiKullanici.ID, msg, 1024, 0);

    cout << "LOG: " << gonderilecekMesaj << endl;
}

int main(){
    LobiOlustur();
    SunucuBaslat();
    return 0;
}
