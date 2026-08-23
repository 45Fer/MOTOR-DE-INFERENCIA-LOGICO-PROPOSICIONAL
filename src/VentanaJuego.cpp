#include "VentanaJuego.h"
#include <chrono>
#include <cmath>
#include <cstdlib>

namespace {
const float ESCALA_AVATAR = 2.5f;
const float ESCALA_CARGA = ESCALA_AVATAR;
const float VELOCIDAD_MOVIMIENTO = 220.0f;
const float DURACION_FRAME = 0.10f;
const float TIEMPO_FRAME_CARGA = 1.0f;
const float GASOLINA_MAXIMA = 100.0f;
const float CONSUMO_GASOLINA_POR_SEGUNDO = 2.0f;
const float CANTIDAD_GASOLINA_RECARGADA = 25.0f;
const float SUENO_MAXIMO = 100.0f;
const float FELICIDAD_MAXIMA = 100.0f;
const float CONSUMO_SUENO_POR_SEGUNDO = 0.8f;
const float CONSUMO_SUENO_CAMINANDO = 1.5f;
const float RECUPERACION_SUENO_DURMIENDO = 4.0f;
const float FELICIDAD_POR_SEGUNDO_CAMINANDO = 2.0f;
const float ENFERMEDAD_MAXIMA = 100.0f;
const float TIEMPO_GRACIA_SIN_NECESIDADES = 10.0f;
const float AUMENTO_ENFERMEDAD_POR_SEGUNDO = 0.35f;
const float AUMENTO_ENFERMEDAD_AMBAS_EN_CERO = 2.0f;
const float CONSUMO_FELICIDAD_POR_SEGUNDO = 0.2f;
const float ESCALA_ACEITE = 0.3f;
const float ESCALA_INDICADOR_GASOLINA = 0.6f;
const float INTERVALO_ACEITE_MINIMO = 4.0f;
const float INTERVALO_ACEITE_MAXIMO = 9.0f;
}

VentanaJuego::VentanaJuego()
    : ventana_(1280, 720, "Tamagotchi - Taller del carro"),
    monitor_(640, 500, "Monitor de estados"),
    texturaFondo_(), fondo_(), texturaAvatar_(), avatar_(),
    texturaReposo_(), texturasCaminar_(), texturasCarga_(),
    texturasDormir_(), texturasMorir_(), texturasEnfermar_(),
    musicaAcelerando_(), musicaArancar_(), musicaLlenar_(),
    musicaPantallaInicio_(), musicaPartida_(), botonRecarga_(),
    botonApagar_(), botonLimpiarAceite_(), texturaAceite_(), manchasAceite_(),
    texturasGasolina_(), indicadorGasolina_(),
    posicionAvatarX_(0.0f), caminando_(false), recargando_(false),
    apagado_(false),
    muriendo_(false),
    indiceAnimacion_(0), indiceCarga_(0), indiceMuerte_(0),
    tiempoAnimacion_(0.0f),
    tiempoCarga_(0.0f), gasolina_(GASOLINA_MAXIMA),
    sueno_(SUENO_MAXIMO), felicidad_(50.0f),
    enfermedad_(0.0f), tiempoSinNecesidades_(0.0f),
    tiempoAceite_(0.0f), siguienteAceite_(INTERVALO_ACEITE_MINIMO),
    audioArancarReproducido_(false), enPantallaInicio_(true) {
    std::srand(static_cast<unsigned int>(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    botonRecarga_.setSize(sf::Vector2f(220.0f, 90.0f));
    botonRecarga_.setFillColor(sf::Color::Transparent);
    botonRecarga_.setOutlineThickness(0.0f);
    botonRecarga_.setPosition(475.0f, 635.0f);
    botonRecarga_.setSize(sf::Vector2f(105.0f, 85.0f));

    botonApagar_.setSize(sf::Vector2f(120.0f, 85.0f));
    botonApagar_.setFillColor(sf::Color::Transparent);
    botonApagar_.setOutlineThickness(0.0f);
    botonApagar_.setPosition(610.0f, 635.0f);

    botonLimpiarAceite_.setSize(sf::Vector2f(120.0f, 85.0f));
    botonLimpiarAceite_.setFillColor(sf::Color::Transparent);
    botonLimpiarAceite_.setOutlineThickness(0.0f);
    botonLimpiarAceite_.setPosition(745.0f, 635.0f);

    botonStart_.setSize(sf::Vector2f(180.0f, 120.0f));
    botonStart_.setFillColor(sf::Color::Transparent);
    botonStart_.setPosition(440.0f, 590.0f);

}

bool VentanaJuego::crear() {
    ventana_.obtenerVentana().setFramerateLimit(60);

    if (!ventana_.estaAbierta() || !cargarFondo() || !cargarAvatar() ||
        !cargarAudio() || !cargarFuente() || !cargarAceite() ||
        !cargarEnfermar() || !cargarPantallaInicio() ||
        !cargarIndicadoresGasolina()) {
        return false;
    }
    return true;
}

int VentanaJuego::ejecutar() {
    sf::Clock reloj;

    while (ventana_.estaAbierta()) {
        const float deltaTiempo = reloj.restart().asSeconds();
        sf::Event evento{};

        procesarEventosMonitor();

        while (ventana_.procesarEvento(evento)) {
            if (evento.type == sf::Event::Closed) {
                musicaAcelerando_.stop();
                musicaArancar_.stop();
                musicaLlenar_.stop();
                musicaPantallaInicio_.stop();
                musicaPartida_.stop();
                ventana_.obtenerVentana().close();
            }
            if (evento.type == sf::Event::MouseButtonPressed &&
                evento.mouseButton.button == sf::Mouse::Left) {
                const sf::Vector2f posicionMouse(
                    static_cast<float>(evento.mouseButton.x),
                    static_cast<float>(evento.mouseButton.y));
                if (enPantallaInicio_) {
                    if (botonStart_.getGlobalBounds().contains(posicionMouse)) {
                        enPantallaInicio_ = false;
                    }
                    continue;
                }
                if (botonApagar_.getGlobalBounds().contains(posicionMouse) &&
                    !muriendo_) {
                    alternarApagado();
                } else if (botonLimpiarAceite_.getGlobalBounds().contains(posicionMouse) &&
                           !muriendo_) {
                    limpiarAceite();
                } else if (botonRecarga_.getGlobalBounds().contains(posicionMouse) &&
                           !apagado_ && !muriendo_) {
                    iniciarRecarga();
                }
            }
        }

        if (enPantallaInicio_) {
            actualizarAudio();
            ventana_.limpiar();
            pintarPantallaInicio();
            ventana_.mostrar();
            pintarMonitor();
            continue;
        }

        const bool moviendoIzquierda = sf::Keyboard::isKeyPressed(sf::Keyboard::A);
        const bool moviendoDerecha = sf::Keyboard::isKeyPressed(sf::Keyboard::D);

        if (!recargando_ && !apagado_ && !muriendo_) {
            if (moviendoIzquierda) {
                moverAvatar(-VELOCIDAD_MOVIMIENTO * deltaTiempo);
            }
            if (moviendoDerecha) {
                moverAvatar(VELOCIDAD_MOVIMIENTO * deltaTiempo);
            }
        }

        if (muriendo_) {
            actualizarMuerte(deltaTiempo);
        } else if (apagado_) {
            actualizarApagado(deltaTiempo);
        } else if (recargando_) {
            actualizarRecarga(deltaTiempo);
        } else {
            actualizarAnimacion(deltaTiempo);
        }
        actualizarGasolina(deltaTiempo);
        actualizarEstado(deltaTiempo);
        actualizarAceite(deltaTiempo);
        actualizarAudio();

        ventana_.limpiar();
        pintarFondo();
        pintarGasolina();
        pintarEstado();
        pintarAceite();
        pintarAvatar();
        ventana_.obtenerVentana().draw(botonRecarga_);
        ventana_.obtenerVentana().draw(botonApagar_);
        ventana_.obtenerVentana().draw(botonLimpiarAceite_);
        ventana_.mostrar();

        if (monitor_.estaAbierta()) {
            pintarMonitor();
        }
    }
    if (monitor_.estaAbierta()) {
        monitor_.obtenerVentana().close();
    }
    return 0;
}

VentanaJuego::EstadoMaquina VentanaJuego::obtenerEstadoMaquina() const {
    if (enPantallaInicio_) {
        return EstadoMaquina::INICIO;
    }
    if (muriendo_) {
        return EstadoMaquina::MURIENDO;
    }
    if (apagado_) {
        return EstadoMaquina::APAGADO;
    }
    if (recargando_) {
        return EstadoMaquina::RECARGANDO;
    }

    const bool necesitaAtencion = sueno_ <= 10.0f || felicidad_ < 25.0f;
    if (necesitaAtencion) {
        return EstadoMaquina::ENFERMO;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
        return EstadoMaquina::CAMINANDO;
    }
    return EstadoMaquina::REPOSO;
}

const char* VentanaJuego::nombreEstado(EstadoMaquina estado) const {
    switch (estado) {
    case EstadoMaquina::INICIO: return "INICIO";
    case EstadoMaquina::REPOSO: return "REPOSO";
    case EstadoMaquina::CAMINANDO: return "CAMINANDO";
    case EstadoMaquina::RECARGANDO: return "RECARGANDO";
    case EstadoMaquina::APAGADO: return "APAGADO";
    case EstadoMaquina::ENFERMO: return "ENFERMO";
    case EstadoMaquina::MURIENDO: return "MURIENDO";
    }
    return "DESCONOCIDO";
}

void VentanaJuego::procesarEventosMonitor() {
    if (!monitor_.estaAbierta()) {
        return;
    }

    sf::Event evento{};
    while (monitor_.procesarEvento(evento)) {
        if (evento.type == sf::Event::Closed) {
            monitor_.obtenerVentana().close();
        }
    }
}

void VentanaJuego::pintarMonitor() {
    const EstadoMaquina estadoActual = obtenerEstadoMaquina();
    sf::RenderWindow& ventanaMonitor = monitor_.obtenerVentana();
    ventanaMonitor.clear(sf::Color(24, 29, 38));

    sf::Text titulo("MAQUINA DE ESTADOS", fuente_);
    titulo.setCharacterSize(22);
    titulo.setFillColor(sf::Color(245, 196, 81));
    titulo.setPosition(22.0f, 16.0f);
    ventanaMonitor.draw(titulo);

    sf::Text actual("ESTADO ACTUAL: ", fuente_);
    actual.setCharacterSize(18);
    actual.setFillColor(sf::Color(210, 218, 230));
    actual.setPosition(350.0f, 19.0f);
    ventanaMonitor.draw(actual);

    sf::Text nombreActual(nombreEstado(estadoActual), fuente_);
    nombreActual.setCharacterSize(18);
    nombreActual.setFillColor(sf::Color(118, 224, 172));
    nombreActual.setPosition(505.0f, 19.0f);
    ventanaMonitor.draw(nombreActual);

    const EstadoMaquina estados[] = {
        EstadoMaquina::INICIO, EstadoMaquina::REPOSO,
        EstadoMaquina::CAMINANDO, EstadoMaquina::RECARGANDO,
        EstadoMaquina::APAGADO, EstadoMaquina::ENFERMO,
        EstadoMaquina::MURIENDO
    };

    const sf::Vector2f posiciones[] = {
        sf::Vector2f(210.0f, 70.0f), sf::Vector2f(210.0f, 145.0f),
        sf::Vector2f(430.0f, 145.0f), sf::Vector2f(75.0f, 240.0f),
        sf::Vector2f(345.0f, 240.0f), sf::Vector2f(75.0f, 335.0f),
        sf::Vector2f(345.0f, 335.0f)
    };

    const int transiciones[][2] = {
        {0, 1}, {1, 2}, {2, 1}, {1, 3}, {3, 1},
        {1, 4}, {4, 1}, {1, 5}, {5, 1}, {5, 6}, {6, 0}
    };
    const auto dibujarFlecha = [&ventanaMonitor, &posiciones](int indiceOrigen,
                                                               int indiceDestino) {
        const sf::Vector2f centroOrigen =
            posiciones[indiceOrigen] + sf::Vector2f(80.0f, 20.0f);
        const sf::Vector2f centroDestino =
            posiciones[indiceDestino] + sf::Vector2f(80.0f, 20.0f);
        const sf::Vector2f diferencia = centroDestino - centroOrigen;
        const float longitud = std::sqrt(
            diferencia.x * diferencia.x + diferencia.y * diferencia.y);
        const sf::Vector2f direccion(diferencia.x / longitud,
                                     diferencia.y / longitud);
        const float distanciaBorde = std::min(
            80.0f / std::max(std::abs(direccion.x), 0.0001f),
            20.0f / std::max(std::abs(direccion.y), 0.0001f));
        const sf::Vector2f inicio = centroOrigen + direccion * distanciaBorde;
        const sf::Vector2f fin = centroDestino - direccion * distanciaBorde;
        const sf::Color colorFlecha(175, 192, 216);
        sf::Vertex linea[] = {
            sf::Vertex(inicio, colorFlecha), sf::Vertex(fin, colorFlecha)
        };
        ventanaMonitor.draw(linea, 2, sf::Lines);

        const sf::Vector2f perpendicular(-direccion.y, direccion.x);
        sf::ConvexShape punta;
        punta.setPointCount(3);
        punta.setPoint(0, fin);
        punta.setPoint(1, fin - direccion * 11.0f + perpendicular * 6.0f);
        punta.setPoint(2, fin - direccion * 11.0f - perpendicular * 6.0f);
        punta.setFillColor(colorFlecha);
        ventanaMonitor.draw(punta);
    };
    for (const auto& transicion : transiciones) {
        dibujarFlecha(transicion[0], transicion[1]);
    }

    for (int indice = 0; indice < 7; ++indice) {
        sf::RectangleShape nodo(sf::Vector2f(160.0f, 40.0f));
        nodo.setPosition(posiciones[indice]);
        nodo.setFillColor(estados[indice] == estadoActual
                               ? sf::Color(43, 139, 102)
                               : sf::Color(42, 49, 62));
        nodo.setOutlineThickness(2.0f);
        nodo.setOutlineColor(estados[indice] == estadoActual
                                 ? sf::Color(118, 224, 172)
                                 : sf::Color(100, 113, 132));
        ventanaMonitor.draw(nodo);

        sf::Text texto(nombreEstado(estados[indice]), fuente_);
        texto.setCharacterSize(16);
        texto.setFillColor(sf::Color::White);
        texto.setPosition(posiciones[indice].x + 14.0f,
                          posiciones[indice].y + 9.0f);
        ventanaMonitor.draw(texto);
    }
    monitor_.mostrar();
}

bool VentanaJuego::cargarFondo() {
    const std::string rutas[] = {
        "assets/fonts/Fondo.png",
        "../assets/fonts/Fondo.png",
        "../../assets/fonts/Fondo.png"
    };

    for (const auto& ruta : rutas) {
        if (texturaFondo_.loadFromFile(ruta)) {
            fondo_.setTexture(texturaFondo_);
            return true;
        }
    }
    return false;
}

bool VentanaJuego::cargarPantallaInicio() {
    const std::string rutas[] = {
        "assets/fonts/Pantalla inicio.png",
        "../assets/fonts/Pantalla inicio.png",
        "../../assets/fonts/Pantalla inicio.png"
    };

    for (const auto& ruta : rutas) {
        if (texturaPantallaInicio_.loadFromFile(ruta)) {
            pantallaInicio_.setTexture(texturaPantallaInicio_);
            return true;
        }
    }
    return false;
}

void VentanaJuego::pintarPantallaInicio() {
    const sf::Vector2u tamanoVentana = ventana_.obtenerVentana().getSize();
    const sf::Vector2u tamanoImagen = texturaPantallaInicio_.getSize();
    pantallaInicio_.setScale(
        static_cast<float>(tamanoVentana.x) / tamanoImagen.x,
        static_cast<float>(tamanoVentana.y) / tamanoImagen.y);
    ventana_.obtenerVentana().draw(pantallaInicio_);
}
bool VentanaJuego::cargarAvatar() {
    texturasCaminar_.clear();

    const std::string rutasReposo[] = {
        "assets/fonts/Reposo.png",
        "../assets/fonts/Reposo.png",
        "../../assets/fonts/Reposo.png"
    };

    for (const auto& ruta : rutasReposo) {
        if (texturaReposo_.loadFromFile(ruta)) {
            break;
        }
    }

    for (int i = 1; i <= 5; ++i) {
        const std::string nombreArchivo = "assets/fonts/Caminar_" + std::to_string(i) + ".png";
        const std::string rutas[] = {
            nombreArchivo,
            "../" + nombreArchivo,
            "../../" + nombreArchivo
        };

        sf::Texture textura;
        bool cargada = false;

        for (const auto& ruta : rutas) {
            if (textura.loadFromFile(ruta)) {
                texturasCaminar_.push_back(textura);
                cargada = true;
                break;
            }
        }

        if (!cargada) {
            return false;
        }
    }

    texturasDormir_.clear();
    for (int i = 1; i <= 4; ++i) {
        const std::string nombreArchivo = "assets/fonts/Dormir_" + std::to_string(i) + ".png";
        const std::string rutas[] = {
            nombreArchivo,
            "../" + nombreArchivo,
            "../../" + nombreArchivo
        };

        sf::Texture textura;
        bool cargada = false;
        for (const auto& ruta : rutas) {
            if (textura.loadFromFile(ruta)) {
                texturasDormir_.push_back(textura);
                cargada = true;
                break;
            }
        }

        if (!cargada) {
            return false;
        }
    }

    texturasMorir_.clear();
    for (int i = 1; i <= 5; ++i) {
        const std::string nombreArchivo = "assets/fonts/Morir_" + std::to_string(i) + ".png";
        const std::string rutas[] = {
            nombreArchivo,
            "../" + nombreArchivo,
            "../../" + nombreArchivo
        };

        sf::Texture textura;
        bool cargada = false;
        for (const auto& ruta : rutas) {
            if (textura.loadFromFile(ruta)) {
                texturasMorir_.push_back(textura);
                cargada = true;
                break;
            }
        }

        if (!cargada) {
            return false;
        }
    }

    if (texturasCaminar_.size() != 5 || texturasMorir_.size() != 5 ||
        !texturaReposo_.getSize().x) {
        return false;
    }

    avatar_.setTexture(texturaReposo_);
    indiceAnimacion_ = 4;
    posicionAvatarX_ =
        (ventana_.obtenerVentana().getSize().x -
         texturaReposo_.getSize().x * ESCALA_AVATAR) / 2.0f;
    return true;
}

bool VentanaJuego::cargarAudio() {
    const std::string rutasAcelerar[] = {
        "assets/music/Acelerar.mp3",
        "../assets/music/Acelerar.mp3",
        "../../assets/music/Acelerar.mp3"
    };

    const std::string rutasArancar[] = {
        "assets/music/Arancar.mp3",
        "../assets/music/Arancar.mp3",
        "../../assets/music/Arancar.mp3"
    };

    const std::string rutasLlenar[] = {
        "assets/music/Llenar.mp3",
        "../assets/music/Llenar.mp3",
        "../../assets/music/Llenar.mp3"
    };

    const std::string rutasPantallaInicio[] = {
        "assets/music/Pantalla de inicio.mp3",
        "../assets/music/Pantalla de inicio.mp3",
        "../../assets/music/Pantalla de inicio.mp3"
    };

    const std::string rutasPartida[] = {
        "assets/music/Partida.mp3",
        "../assets/music/Partida.mp3",
        "../../assets/music/Partida.mp3"
    };

    bool audioAcelerarOK = false;
    for (const auto& ruta : rutasAcelerar) {
        if (musicaAcelerando_.openFromFile(ruta)) {
            musicaAcelerando_.setLoop(true);
            musicaAcelerando_.setVolume(50.f);
            audioAcelerarOK = true;
            break;
        }
    }

    bool audioArancarOK = false;
    for (const auto& ruta : rutasArancar) {
        if (musicaArancar_.openFromFile(ruta)) {
            musicaArancar_.setVolume(80.f);
            audioArancarOK = true;
            break;
        }
    }

    bool audioLlenarOK = false;
    for (const auto& ruta : rutasLlenar) {
        if (musicaLlenar_.openFromFile(ruta)) {
            musicaLlenar_.setLoop(true);
            musicaLlenar_.setVolume(80.f);
            audioLlenarOK = true;
            break;
        }
    }

    bool audioPantallaInicioOK = false;
    for (const auto& ruta : rutasPantallaInicio) {
        if (musicaPantallaInicio_.openFromFile(ruta)) {
            musicaPantallaInicio_.setLoop(true);
            musicaPantallaInicio_.setVolume(80.f);
            audioPantallaInicioOK = true;
            break;
        }
    }

    bool audioPartidaOK = false;
    for (const auto& ruta : rutasPartida) {
        if (musicaPartida_.openFromFile(ruta)) {
            musicaPartida_.setLoop(true);
            musicaPartida_.setVolume(55.f);
            audioPartidaOK = true;
            break;
        }
    }

    return audioAcelerarOK && audioArancarOK && audioLlenarOK &&
           audioPantallaInicioOK && audioPartidaOK;
}

bool VentanaJuego::cargarFuente() {
    const std::string rutas[] = {
        "C:/Windows/Fonts/arial.ttf",
        "../C:/Windows/Fonts/arial.ttf"
    };

    for (const auto& ruta : rutas) {
        if (fuente_.loadFromFile(ruta)) {
            textoSueno_.setFont(fuente_);
            textoFelicidad_.setFont(fuente_);
            textoSueno_.setCharacterSize(26);
            textoFelicidad_.setCharacterSize(26);
            textoSueno_.setFillColor(sf::Color::White);
            textoFelicidad_.setFillColor(sf::Color::White);
            textoSueno_.setPosition(20.0f, 20.0f);
            textoFelicidad_.setPosition(20.0f, 55.0f);
            return true;
        }
    }
    return false;
}

bool VentanaJuego::cargarIndicadoresGasolina() {
    const std::string nombres[] = {
        "Lleno.png",
        "Casi Lleno.png",
        "Medio.png",
        "Bajo.png",
        "Muy bajo.png",
        "Vacio.png"
    };

    texturasGasolina_.clear();
    for (const auto& nombre : nombres) {
        const std::string rutas[] = {
            "assets/fonts/" + nombre,
            "../assets/fonts/" + nombre,
            "../../assets/fonts/" + nombre
        };

        sf::Texture textura;
        bool cargada = false;
        for (const auto& ruta : rutas) {
            if (textura.loadFromFile(ruta)) {
                texturasGasolina_.push_back(textura);
                cargada = true;
                break;
            }
        }

        if (!cargada) {
            return false;
        }
    }

    indicadorGasolina_.setTexture(texturasGasolina_[0]);
    return true;
}

void VentanaJuego::pintarFondo() {
    const sf::Vector2u tamanoVentana = ventana_.obtenerVentana().getSize();
    const sf::Vector2u tamanoImagen = texturaFondo_.getSize();
    fondo_.setScale(
        static_cast<float>(tamanoVentana.x) / tamanoImagen.x,
        static_cast<float>(tamanoVentana.y) / tamanoImagen.y);
    ventana_.obtenerVentana().draw(fondo_);
}

void VentanaJuego::pintarAvatar() {
    const sf::Vector2u tamanoVentana = ventana_.obtenerVentana().getSize();
    const sf::Vector2u tamanoAvatar = avatar_.getTexture()->getSize();
    const float offsetY = recargando_ ? 110.0f : 140.0f;
    const float escala = recargando_ ? ESCALA_CARGA : ESCALA_AVATAR;

    avatar_.setScale(escala, escala);
    avatar_.setPosition(
        posicionAvatarX_,
        static_cast<float>(tamanoVentana.y) -
            tamanoAvatar.y * escala - offsetY);
    ventana_.obtenerVentana().draw(avatar_);
}

void VentanaJuego::pintarGasolina() {
    std::size_t indice = 5;
    if (gasolina_ >= 85.0f) {
        indice = 0;
    } else if (gasolina_ >= 65.0f) {
        indice = 1;
    } else if (gasolina_ >= 45.0f) {
        indice = 2;
    } else if (gasolina_ >= 25.0f) {
        indice = 3;
    } else if (gasolina_ > 0.0f) {
        indice = 4;
    }

    indicadorGasolina_.setTexture(texturasGasolina_[indice]);
    indicadorGasolina_.setScale(ESCALA_INDICADOR_GASOLINA,
                                ESCALA_INDICADOR_GASOLINA);
    const float posicionX =
        static_cast<float>(ventana_.obtenerVentana().getSize().x) -
        static_cast<float>(indicadorGasolina_.getTexture()->getSize().x) *
            ESCALA_INDICADOR_GASOLINA - 20.0f;
    indicadorGasolina_.setPosition(posicionX, 20.0f);
    ventana_.obtenerVentana().draw(indicadorGasolina_);
}

void VentanaJuego::pintarEstado() {
    textoSueno_.setString("Sueno: " + std::to_string(static_cast<int>(sueno_)) + "%");
    textoFelicidad_.setString("Felicidad: " + std::to_string(static_cast<int>(felicidad_)) + "%");
    ventana_.obtenerVentana().draw(textoSueno_);
    ventana_.obtenerVentana().draw(textoFelicidad_);
}

void VentanaJuego::moverAvatar(float distancia) {
    const float anchoVentana =
        static_cast<float>(ventana_.obtenerVentana().getSize().x);
    const float anchoAvatar = texturasCaminar_[0].getSize().x * ESCALA_AVATAR;

    posicionAvatarX_ += distancia;
    if (posicionAvatarX_ < 0.0f) {
        posicionAvatarX_ = 0.0f;
    }
    if (posicionAvatarX_ + anchoAvatar > anchoVentana) {
        posicionAvatarX_ = anchoVentana - anchoAvatar;
    }
}

void VentanaJuego::actualizarAnimacion(float deltaTiempo) {
    if (texturasCaminar_.empty()) {
        return;
    }

    const bool moviendo = sf::Keyboard::isKeyPressed(sf::Keyboard::A) ||
                          sf::Keyboard::isKeyPressed(sf::Keyboard::D);

    if (!moviendo) {
        if (caminando_) {
            caminando_ = false;
            avatar_.setTexture(texturaReposo_);
        } else {
            avatar_.setTexture(texturaReposo_);
        }
        return;
    }

    if (!caminando_) {
        caminando_ = true;
        indiceAnimacion_ = 0;
        tiempoAnimacion_ = 0.0f;
        avatar_.setTexture(texturasCaminar_[indiceAnimacion_]);
        return;
    }

    tiempoAnimacion_ += deltaTiempo;
    if (tiempoAnimacion_ >= DURACION_FRAME) {
        tiempoAnimacion_ = 0.0f;
        if (indiceAnimacion_ < 4) {
            ++indiceAnimacion_;
        }
        avatar_.setTexture(texturasCaminar_[indiceAnimacion_]);
    }
}

void VentanaJuego::actualizarAudio() {
    if (enPantallaInicio_) {
        musicaPartida_.stop();
        musicaAcelerando_.stop();
        if (musicaPantallaInicio_.getStatus() != sf::Music::Playing) {
            musicaPantallaInicio_.play();
        }
        return;
    }

    musicaPantallaInicio_.stop();
    if (musicaPartida_.getStatus() != sf::Music::Playing) {
        musicaPartida_.play();
    }

    if (recargando_ || apagado_ || muriendo_) {
        if (musicaAcelerando_.getStatus() == sf::Music::Playing) {
            musicaAcelerando_.stop();
        }
        return;
    }

    const bool moviendo = sf::Keyboard::isKeyPressed(sf::Keyboard::A) ||
                          sf::Keyboard::isKeyPressed(sf::Keyboard::D);

    if (moviendo) {
        if (musicaAcelerando_.getStatus() != sf::Music::Playing) {
            musicaAcelerando_.play();
        }
    } else if (musicaAcelerando_.getStatus() == sf::Music::Playing) {
        musicaAcelerando_.stop();
    }
}

void VentanaJuego::actualizarGasolina(float deltaTiempo) {
    if (!recargando_ && !apagado_ && !muriendo_) {
        gasolina_ -= CONSUMO_GASOLINA_POR_SEGUNDO * deltaTiempo;
    }

    if (gasolina_ < 0.0f) {
        gasolina_ = 0.0f;
    }
    if (gasolina_ == 0.0f && !muriendo_ && !apagado_) {
        iniciarMuerte();
    }
}

void VentanaJuego::iniciarRecarga() {
    if (recargando_) {
        return;
    }

    recargando_ = true;
    indiceCarga_ = 0;
    tiempoCarga_ = 0.0f;
    audioArancarReproducido_ = false;
    musicaAcelerando_.stop();
    musicaLlenar_.play();
    texturasCarga_.clear();

    for (int i = 1; i <= 6; ++i) {
        const std::string nombreArchivo = "assets/fonts/Cargar_" + std::to_string(i) + ".png";
        const std::string rutas[] = {
            nombreArchivo,
            "../" + nombreArchivo,
            "../../" + nombreArchivo
        };

        sf::Texture textura;
        bool cargada = false;
        for (const auto& ruta : rutas) {
            if (textura.loadFromFile(ruta)) {
                texturasCarga_.push_back(textura);
                cargada = true;
                break;
            }
        }

        if (!cargada) {
            recargando_ = false;
            musicaLlenar_.stop();
            return;
        }
    }

    if (!texturasCarga_.empty()) {
        avatar_.setTexture(texturasCarga_[0]);
        posicionAvatarX_ =
            (ventana_.obtenerVentana().getSize().x -
             texturasCarga_[0].getSize().x * ESCALA_CARGA) / 2.0f;
    }
}

void VentanaJuego::actualizarRecarga(float deltaTiempo) {
    if (texturasCarga_.empty()) {
        recargando_ = false;
        musicaLlenar_.stop();
        avatar_.setTexture(texturaReposo_);
        return;
    }

    tiempoCarga_ += deltaTiempo;
    if (tiempoCarga_ >= TIEMPO_FRAME_CARGA) {
        tiempoCarga_ = 0.0f;
        if (indiceCarga_ < 5) {
            ++indiceCarga_;
        }
        avatar_.setTexture(texturasCarga_[indiceCarga_]);

        if (indiceCarga_ == 5) {
            if (!audioArancarReproducido_) {
                musicaArancar_.play();
                audioArancarReproducido_ = true;
            }
        }

        if (indiceCarga_ >= 5 && audioArancarReproducido_) {
            gasolina_ += CANTIDAD_GASOLINA_RECARGADA;
            if (gasolina_ > GASOLINA_MAXIMA) {
                gasolina_ = GASOLINA_MAXIMA;
            }
            recargando_ = false;
            musicaLlenar_.stop();
            avatar_.setTexture(texturaReposo_);
            musicaArancar_.play();
            return;
        }
    }
}

void VentanaJuego::alternarApagado() {
    apagado_ = !apagado_;
    caminando_ = false;
    tiempoAnimacion_ = 0.0f;

    if (apagado_) {
        recargando_ = false;
        musicaAcelerando_.stop();
        musicaArancar_.stop();
        musicaLlenar_.stop();
        indiceAnimacion_ = 0;
        avatar_.setTexture(texturasDormir_[indiceAnimacion_]);
    } else {
        avatar_.setTexture(texturaReposo_);
    }
}

void VentanaJuego::iniciarMuerte() {
    if (texturasMorir_.size() < 5) {
        return;
    }

    muriendo_ = true;
    recargando_ = false;
    apagado_ = false;
    caminando_ = false;
    indiceMuerte_ = 0;
    tiempoAnimacion_ = 0.0f;
    musicaAcelerando_.stop();
    musicaArancar_.stop();
    musicaLlenar_.stop();
    avatar_.setTexture(texturasMorir_[indiceMuerte_]);
}

void VentanaJuego::actualizarMuerte(float deltaTiempo) {
    tiempoAnimacion_ += deltaTiempo;
    if (tiempoAnimacion_ < TIEMPO_FRAME_CARGA) {
        return;
    }

    tiempoAnimacion_ = 0.0f;
    ++indiceMuerte_;
    if (indiceMuerte_ < 5) {
        avatar_.setTexture(texturasMorir_[indiceMuerte_]);
        return;
    }

    muriendo_ = false;
    gasolina_ = GASOLINA_MAXIMA;
    sueno_ = SUENO_MAXIMO;
    felicidad_ = 50.0f;
    enfermedad_ = 0.0f;
    tiempoSinNecesidades_ = 0.0f;
    enPantallaInicio_ = true;
    limpiarAceite();
    tiempoAceite_ = 0.0f;
    siguienteAceite_ = INTERVALO_ACEITE_MINIMO;
    indiceAnimacion_ = 4;
    avatar_.setTexture(texturaReposo_);
    posicionAvatarX_ =
        (ventana_.obtenerVentana().getSize().x -
         texturaReposo_.getSize().x * ESCALA_AVATAR) / 2.0f;
}

void VentanaJuego::actualizarApagado(float deltaTiempo) {
    if (texturasDormir_.size() < 4) {
        return;
    }

    tiempoAnimacion_ += deltaTiempo;
    if (tiempoAnimacion_ >= TIEMPO_FRAME_CARGA) {
        tiempoAnimacion_ = 0.0f;
        if (indiceAnimacion_ < 3) {
            ++indiceAnimacion_;
        } else {
            indiceAnimacion_ = indiceAnimacion_ == 2 ? 3 : 2;
        }
        avatar_.setTexture(texturasDormir_[indiceAnimacion_]);
    }
}

void VentanaJuego::actualizarEstado(float deltaTiempo) {
    if (apagado_) {
        sueno_ += RECUPERACION_SUENO_DURMIENDO * deltaTiempo;
        if (sueno_ > SUENO_MAXIMO) {
            sueno_ = SUENO_MAXIMO;
        }
        return;
    }

    if (recargando_ || muriendo_) {
        return;
    }

    const bool moviendo = sf::Keyboard::isKeyPressed(sf::Keyboard::A) ||
                          sf::Keyboard::isKeyPressed(sf::Keyboard::D);
    sueno_ -= (CONSUMO_SUENO_POR_SEGUNDO +
               (moviendo ? CONSUMO_SUENO_CAMINANDO : 0.0f)) * deltaTiempo;
    felicidad_ -= CONSUMO_FELICIDAD_POR_SEGUNDO * deltaTiempo;
    if (moviendo) {
        felicidad_ += FELICIDAD_POR_SEGUNDO_CAMINANDO * deltaTiempo;
    }

    if (sueno_ < 0.0f) {
        sueno_ = 0.0f;
    }
    if (felicidad_ > FELICIDAD_MAXIMA) {
        felicidad_ = FELICIDAD_MAXIMA;
    }
    if (felicidad_ < 0.0f) {
        felicidad_ = 0.0f;
    }

    const bool suenoAgotado = sueno_ <= 0.0f;
    const bool felicidadAgotada = felicidad_ <= 0.0f;
    const bool necesitaAtencion = sueno_ <= 10.0f || felicidad_ < 25.0f;

    if (suenoAgotado && felicidadAgotada) {
        tiempoSinNecesidades_ += deltaTiempo;
        enfermedad_ += AUMENTO_ENFERMEDAD_AMBAS_EN_CERO * deltaTiempo;
        if (tiempoSinNecesidades_ >= TIEMPO_GRACIA_SIN_NECESIDADES) {
            enfermedad_ = ENFERMEDAD_MAXIMA;
        }
    } else {
        tiempoSinNecesidades_ = 0.0f;
        if (necesitaAtencion) {
            enfermedad_ += AUMENTO_ENFERMEDAD_POR_SEGUNDO * deltaTiempo;
        } else {
            enfermedad_ -= AUMENTO_ENFERMEDAD_POR_SEGUNDO * deltaTiempo;
        }
    }

    if (enfermedad_ < 0.0f) {
        enfermedad_ = 0.0f;
    }
    if (enfermedad_ >= ENFERMEDAD_MAXIMA) {
        iniciarMuerte();
        return;
    }

    if (necesitaAtencion) {
        std::size_t indiceEnfermedad = 0;
        if (suenoAgotado && felicidadAgotada) {
            indiceEnfermedad = 3;
        } else if (suenoAgotado && felicidad_ < 25.0f) {
            indiceEnfermedad = 2;
        } else if (suenoAgotado) {
            indiceEnfermedad = 1;
        }
        avatar_.setTexture(texturasEnfermar_[indiceEnfermedad]);
    }
}

void VentanaJuego::actualizarAceite(float deltaTiempo) {
    if (apagado_ || recargando_ || muriendo_) {
        return;
    }

    tiempoAceite_ += deltaTiempo;
    if (tiempoAceite_ < siguienteAceite_) {
        return;
    }

    tiempoAceite_ = 0.0f;
    const float proporcion = static_cast<float>(std::rand()) /
                             static_cast<float>(RAND_MAX);
    siguienteAceite_ = INTERVALO_ACEITE_MINIMO +
                       proporcion * (INTERVALO_ACEITE_MAXIMO -
                                     INTERVALO_ACEITE_MINIMO);

    sf::Sprite mancha(texturaAceite_);
    mancha.setScale(ESCALA_ACEITE, ESCALA_ACEITE);
    mancha.setPosition(
        posicionAvatarX_,
        static_cast<float>(ventana_.obtenerVentana().getSize().y) -
            texturaAceite_.getSize().y * ESCALA_ACEITE - 55.0f);
    manchasAceite_.push_back(mancha);
}

void VentanaJuego::limpiarAceite() {
    manchasAceite_.clear();
}

bool VentanaJuego::cargarAceite() {
    const std::string rutas[] = {
        "assets/fonts/Aceite.png",
        "../assets/fonts/Aceite.png",
        "../../assets/fonts/Aceite.png"
    };

    for (const auto& ruta : rutas) {
        if (texturaAceite_.loadFromFile(ruta)) {
            return true;
        }
    }
    return false;
}

bool VentanaJuego::cargarEnfermar() {
    texturasEnfermar_.clear();
    for (int i = 1; i <= 4; ++i) {
        const std::string nombreArchivo = "assets/fonts/Enfermar_" +
                                          std::to_string(i) + ".png";
        const std::string rutas[] = {
            nombreArchivo,
            "../" + nombreArchivo,
            "../../" + nombreArchivo
        };

        sf::Texture textura;
        bool cargada = false;
        for (const auto& ruta : rutas) {
            if (textura.loadFromFile(ruta)) {
                texturasEnfermar_.push_back(textura);
                cargada = true;
                break;
            }
        }

        if (!cargada) {
            return false;
        }
    }
    return true;
}

void VentanaJuego::pintarAceite() {
    for (const auto& mancha : manchasAceite_) {
        ventana_.obtenerVentana().draw(mancha);
    }
}