Zu erreichende Ziele
- [x] �berpr�fen, ob alle Kommanden[^1] im Manual landen.
- [x] Manual f�r Installationen m�glichst fertig schreiben.
- [ ] Komponentendiagramm
    - [ ] Mit eigenen Komponenten erweitern.
    - [ ] Text f�r das Verst�dnis hinzuf�gen.
- [ ] PowerPoint Pr�sentationen
    - [ ] Die letzte Pr�sentation erg�nzen mit Infos.
    - [ ] Pr�sentation f�r 17.07.2023

Optionale Ziele
- [x] Test f�r Programm verstehen. [Zu den Hinweisen f�r die Tests](#Test)
- [ ] Einen kleinen Test schreiben.
    - [ ] Schauen, ob es m�glich ist Logs anzulegen und wichtige Informationen darin zu hinterlegen.
- [x] Im Ordner gtest nach hilfreichen Informationen durchsuchen.

## <a id='Test'>Test</a>

GTest ist eine API zum Testen von Funktionen
Der Ordner ***conf*** stellt die Konfiguration des Emulators bereit, die mindestens notwendig sind um den Emulator lauff�hig zu machen.

Es stehen auch Dokumentationen f�r die Funktionen aus gtest.
Z.B. steht f�r TEST_P parametisierten Aufruf w�hrend TEST_F ein nicht-parametisierter Aufruf entspricht.
```cpp
// A parameterized test fixture must be derived from testing::Test and from
// testing::WithParamInterface<T>, where T is the type of the parameter
// values. Inheriting from TestWithParam<T> satisfies that requirement because
// TestWithParam<T> inherits from both Test and WithParamInterface. In more
// complicated hierarchies, however, it is occasionally useful to inherit
// separately from Test and WithParamInterface. For example:

class BaseTest : public ::testing::Test {
  // You can inherit all the usual members for a non-parameterized test
  // fixture here.
};

class DerivedTest : public BaseTest, public ::testing::WithParamInterface<int> {
  // The usual test fixture members go here too.
};

TEST_F(BaseTest, HasFoo) {
  // This is an ordinary non-parameterized test.
}

TEST_P(DerivedTest, DoesBlah) {
  // GetParam works just the same here as if you inherit from TestWithParam.
  EXPECT_TRUE(foo.Blah(GetParam()));
}
```


[^1]: Kommanden
```bash
user@system: PathToLibpaxos$ mkdir build
user@system: PathToLibpaxos$ cd build
user@system: PathToLibpaxos/build$ sudo apt-get install build-essential
user@system: PathToLibpaxos/build$ sudo apt-get install libevent-dev
user@system: PathToLibpaxos/build$ sudo apt-get install libmsgpack-dev
user@system: PathToLibpaxos/build$ sudo apt-get install libgtest-dev
user@system: PathToLibpaxos/build$ cmake ..
user@system: PathToLibpaxos/build$ make
```
config_unittest.cc
replica_unittest.cc