#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include "Map.h"
#include "Structure.h"
#include "Noise/noise.h"
#include "QuadTree.h"
#include "PoissonDiskSampling/PoissonDiskSampling.h"

const int WIDTH = 800;
const int HEIGHT = 600;

const int POINT_SIZE = 2;
const int LINE_SIZE = 1;

struct InfoShown
{
	enum class Name
	{
		Elevation,
		Moisture,
		Biomes
	};
};

InfoShown::Name VideoMode;

sf::Color DELAUNAY_COLOR = sf::Color::Black;
sf::Color VORONOI_COLOR = sf::Color(sf::Uint8(52), sf::Uint8(58), sf::Uint8(94), sf::Uint8(127));
sf::Color WATER_COLOR = sf::Color(sf::Uint8(52), sf::Uint8(58), sf::Uint8(94));
sf::Color LAND_COLOR = sf::Color(sf::Uint8(178), sf::Uint8(166), sf::Uint8(148));
sf::Color LAKE_COLOR = sf::Color(sf::Uint8(95), sf::Uint8(134), sf::Uint8(169));
sf::Color RIVER_COLOR = sf::Color(sf::Uint8(40), sf::Uint8(88), sf::Uint8(132));

const sf::Color ELEVATION_COLOR[] =
{
	sf::Color(sf::Uint8(104), sf::Uint8(134), sf::Uint8(89)),
	sf::Color(sf::Uint8(119), sf::Uint8(153), sf::Uint8(102)),
	sf::Color(sf::Uint8(136), sf::Uint8(166), sf::Uint8(121)),
	sf::Color(sf::Uint8(153), sf::Uint8(179), sf::Uint8(148)),
	sf::Color(sf::Uint8(170), sf::Uint8(191), sf::Uint8(159)),
	sf::Color(sf::Uint8(187), sf::Uint8(204), sf::Uint8(179)),
	sf::Color(sf::Uint8(204), sf::Uint8(217), sf::Uint8(198)),
	sf::Color(sf::Uint8(221), sf::Uint8(230), sf::Uint8(217)),
	sf::Color(sf::Uint8(238), sf::Uint8(242), sf::Uint8(236)),
	sf::Color(sf::Uint8(251), sf::Uint8(252), sf::Uint8(251))
};

const sf::Color MOISTURE_COLOR[] =
{
	sf::Color(sf::Uint8(238), sf::Uint8(238), sf::Uint8(32)),
	sf::Color(sf::Uint8(218), sf::Uint8(238), sf::Uint8(32)),
	sf::Color(sf::Uint8(197), sf::Uint8(238), sf::Uint8(32)),
	sf::Color(sf::Uint8(176), sf::Uint8(238), sf::Uint8(32)),
	sf::Color(sf::Uint8(155), sf::Uint8(238), sf::Uint8(32)),
	sf::Color(sf::Uint8(135), sf::Uint8(238), sf::Uint8(32)),
	sf::Color(sf::Uint8(115), sf::Uint8(238), sf::Uint8(32)),
	sf::Color(sf::Uint8(94), sf::Uint8(238), sf::Uint8(32)),
	sf::Color(sf::Uint8(73), sf::Uint8(238), sf::Uint8(32)),
	sf::Color(sf::Uint8(52), sf::Uint8(238), sf::Uint8(32)),
	sf::Color(sf::Uint8(32), sf::Uint8(238), sf::Uint8(32))
};

const sf::Color BIOME_COLOR[] =
{
	sf::Color(sf::Uint8(248), sf::Uint8(248), sf::Uint8(248)),
	sf::Color(sf::Uint8(221), sf::Uint8(221), sf::Uint8(187)),
	sf::Color(sf::Uint8(153), sf::Uint8(153), sf::Uint8(153)),
	sf::Color(sf::Uint8(204), sf::Uint8(212), sf::Uint8(187)),
	sf::Color(sf::Uint8(196), sf::Uint8(204), sf::Uint8(187)),
	sf::Color(sf::Uint8(228), sf::Uint8(232), sf::Uint8(202)),
	sf::Color(sf::Uint8(164), sf::Uint8(196), sf::Uint8(168)),
	sf::Color(sf::Uint8(180), sf::Uint8(201), sf::Uint8(169)),
	sf::Color(sf::Uint8(196), sf::Uint8(212), sf::Uint8(170)),
	sf::Color(sf::Uint8(156), sf::Uint8(187), sf::Uint8(169)),
	sf::Color(sf::Uint8(169), sf::Uint8(204), sf::Uint8(164)),
	sf::Color(sf::Uint8(233), sf::Uint8(221), sf::Uint8(199)),
	sf::Color(sf::Uint8(52), sf::Uint8(58), sf::Uint8(94)),
	sf::Color(sf::Uint8(95), sf::Uint8(134), sf::Uint8(169)),
	sf::Color(sf::Uint8(178), sf::Uint8(166), sf::Uint8(148))
};

void DrawLine(Vector2 a, Vector2 b, double width, sf::Color c, sf::RenderWindow *window);
void DrawEdge(Edge* e, sf::RenderWindow* window);
void DrawCorner(Corner* c, sf::RenderWindow* window);
void DrawCenter(Center* c, sf::RenderWindow* window);