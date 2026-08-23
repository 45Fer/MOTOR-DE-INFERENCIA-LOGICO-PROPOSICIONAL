#pragma once

#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <vector>
#include "Ventana.hpp"

class VentanaJuego {
public:
    VentanaJuego();
    bool crear();
    int ejecutar();

private:
    enum class EstadoMaquina {
        INICIO,
        REPOSO,
        CAMINANDO,
        RECARGANDO,
        APAGADO,
        ENFERMO,
        MURIENDO
    };

    bool cargarFondo();
    bool cargarAvatar();
    bool cargarAudio();
    bool cargarFuente();
    bool cargarIndicadoresGasolina();
    bool cargarAceite();
    bool cargarEnfermar();
    bool cargarPantallaInicio();
    void pintarFondo();
    void pintarAvatar();
    void pintarGasolina();
    void pintarEstado();
    void pintarAceite();
    void pintarPantallaInicio();
    void moverAvatar(float distancia);
    void actualizarAnimacion(float deltaTiempo);
    void actualizarAudio();
    void actualizarGasolina(float deltaTiempo);
    void actualizarEstado(float deltaTiempo);
    void actualizarAceite(float deltaTiempo);
    void limpiarAceite();
    void iniciarRecarga();
    void actualizarRecarga(float deltaTiempo);
    void actualizarApagado(float deltaTiempo);
    void alternarApagado();
    void actualizarMuerte(float deltaTiempo);
    void iniciarMuerte();
    EstadoMaquina obtenerEstadoMaquina() const;
    const char* nombreEstado(EstadoMaquina estado) const;
    void procesarEventosMonitor();
    void pintarMonitor();

    Ventana ventana_;
    Ventana monitor_;
    sf::Texture texturaFondo_;
    sf::Sprite fondo_;
    sf::Texture texturaAvatar_;
    sf::Sprite avatar_;
    sf::Texture texturaReposo_;
    std::vector<sf::Texture> texturasCaminar_;
    std::vector<sf::Texture> texturasCarga_;
    std::vector<sf::Texture> texturasDormir_;
    std::vector<sf::Texture> texturasMorir_;
    std::vector<sf::Texture> texturasEnfermar_;
    sf::Music musicaAcelerando_;
    sf::Music musicaArancar_;
    sf::Music musicaLlenar_;
    sf::Music musicaPantallaInicio_;
    sf::Music musicaPartida_;
    sf::Font fuente_;
    sf::Text textoSueno_;
    sf::Text textoFelicidad_;
    sf::RectangleShape botonRecarga_;
    sf::RectangleShape botonApagar_;
    sf::RectangleShape botonLimpiarAceite_;
    sf::Texture texturaAceite_;
    std::vector<sf::Sprite> manchasAceite_;
    std::vector<sf::Texture> texturasGasolina_;
    sf::Sprite indicadorGasolina_;
    float posicionAvatarX_;
    bool caminando_;
    bool recargando_;
    bool apagado_;
    bool muriendo_;
    int indiceAnimacion_;
    int indiceCarga_;
    int indiceMuerte_;
    float tiempoAnimacion_;
    float tiempoCarga_;
    float gasolina_;
    float sueno_;
    float felicidad_;
    float enfermedad_;
    float tiempoSinNecesidades_;
    float tiempoAceite_;
    float siguienteAceite_;
    bool audioArancarReproducido_;
    sf::Texture texturaPantallaInicio_;
    sf::Sprite pantallaInicio_;
    sf::RectangleShape botonStart_;
    bool enPantallaInicio_;
};