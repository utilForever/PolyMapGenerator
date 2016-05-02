#include <iostream>
#include <random>
#include <queue>
#include <SFML/System.hpp>

#include "Map.h"
#include "PoissonDiskSampling/PoissonDiskSampling.h"
#include "Math/Vector2.h"
#include "Noise/Noise.h"

const std::vector<std::vector<BiomeType>> Map::m_elevationMoistureMatrix = MakeBiomeMatrix();

std::vector<std::vector<BiomeType>> Map::MakeBiomeMatrix()
{
	std::vector<std::vector<BiomeType>> matrix;
	std::vector<BiomeType> biomeVector;

	biomeVector.push_back(BiomeType::SubtropicalDesert);
	biomeVector.push_back(BiomeType::TemprateDesert);
	biomeVector.push_back(BiomeType::TemprateDesert);
	biomeVector.push_back(BiomeType::Mountain);
	matrix.push_back(biomeVector);
	biomeVector.clear();

	biomeVector.push_back(BiomeType::Grassland);
	biomeVector.push_back(BiomeType::Grassland);
	biomeVector.push_back(BiomeType::TemprateDesert);
	biomeVector.push_back(BiomeType::Mountain);
	matrix.push_back(biomeVector);
	biomeVector.clear();

	biomeVector.push_back(BiomeType::TropicalSeasonalForest);
	biomeVector.push_back(BiomeType::Grassland);
	biomeVector.push_back(BiomeType::Shrubland);
	biomeVector.push_back(BiomeType::Tundra);
	matrix.push_back(biomeVector);
	biomeVector.clear();

	biomeVector.push_back(BiomeType::TropicalSeasonalForest);
	biomeVector.push_back(BiomeType::TemprateDeciduousForest);
	biomeVector.push_back(BiomeType::Shrubland);
	biomeVector.push_back(BiomeType::Snow);
	matrix.push_back(biomeVector);
	biomeVector.clear();

	biomeVector.push_back(BiomeType::TropicalRainForest);
	biomeVector.push_back(BiomeType::TemprateDeciduousForest);
	biomeVector.push_back(BiomeType::Taiga);
	biomeVector.push_back(BiomeType::Snow);
	matrix.push_back(biomeVector);
	biomeVector.clear();

	biomeVector.push_back(BiomeType::TropicalRainForest);
	biomeVector.push_back(BiomeType::TemprateRainForest);
	biomeVector.push_back(BiomeType::Taiga);
	biomeVector.push_back(BiomeType::Snow);
	matrix.push_back(biomeVector);
	biomeVector.clear();

	return matrix;
}

Map::Map(int width, int height, double pointSpread, std::string seed) :
	m_mapWidth(width), m_mapHeight(height), m_pointSpread(pointSpread), m_zCoord(0.0),
	m_noiseMap(nullptr), m_seed(seed), m_centersQuadTree(AABB(Vector2(width / 2, height / 2), Vector2(width / 2, height / 2)), 1)
{
	double approxPointCount = (2 * m_mapWidth * m_mapHeight) / (3.1416 * m_pointSpread * m_pointSpread);
	int maxTreeDepth = floor((log(approxPointCount) / log(4)) + 0.5);
	QuadTree<Center*>::SetMaxDepth(maxTreeDepth);

	m_seed = seed != "" ? seed : CreateSeed(20);
	std::mt19937 mt_rand(HashString(m_seed));

	m_zCoord = mt_rand();
	std::cout << "Seed: " << m_seed << "(" << HashString(m_seed) << ")" << std::endl;
}

void Map::Generate()
{
	sf::Clock timer;

	GeneratePolygons();

	std::cout << "Land distribution: ";
	timer.restart();
	GenerateLand();
	std::cout << timer.getElapsedTime().asMicroseconds() / 1000.0 << " ms." << std::endl;


	// Elevation
	std::cout << "Coast assignment: ";
	timer.restart();
	AssignOceanCoastLand();
	std::cout << timer.getElapsedTime().asMicroseconds() / 1000.0 << " ms." << std::endl;

	std::cout << "Corner altitude: ";
	timer.restart();
	AssignCornerElevations();
	std::cout << timer.getElapsedTime().asMicroseconds() / 1000.0 << " ms." << std::endl;

	std::cout << "Altitude redistribution: ";
	timer.restart();
	RedistributeElevations();
	std::cout << timer.getElapsedTime().asMicroseconds() / 1000.0 << " ms." << std::endl;

	std::cout << "Center altitude: ";
	timer.restart();
	AssignPolygonElevations();
	std::cout << timer.getElapsedTime().asMicroseconds() / 1000.0 << " ms." << std::endl;

	// Moisture
	std::cout << "Downslopes: ";
	timer.restart();
	CalculateDownslopes();
	std::cout << timer.getElapsedTime().asMicroseconds() / 1000.0 << " ms." << std::endl;

	std::cout << "River generation: ";
	timer.restart();
	GenerateRivers();
	std::cout << timer.getElapsedTime().asMicroseconds() / 1000.0 << " ms." << std::endl;

	std::cout << "Corner moisture: ";
	timer.restart();
	AssignCornerMoisture();
	std::cout << timer.getElapsedTime().asMicroseconds() / 1000.0 << " ms." << std::endl;

	std::cout << "Moisture redistribution: ";
	timer.restart();
	RedistributeMoisture();
	std::cout << timer.getElapsedTime().asMicroseconds() / 1000.0 << " ms." << std::endl;

	std::cout << "Center moisture: ";
	timer.restart();
	AssignPolygonMoisture();
	std::cout << timer.getElapsedTime().asMicroseconds() / 1000.0 << " ms." << std::endl;

	// Biomes
	std::cout << "Biome assignment: ";
	timer.restart();
	AssignBiomes();
	std::cout << timer.getElapsedTime().asMicroseconds() / 1000.0 << " ms." << std::endl;

	std::cout << "Populate Quadtree: ";
	timer.restart();
	for (auto center : m_centers)
	{
		std::pair<Vector2, Vector2> aabb(center->GetBoundingBox());
		m_centersQuadTree.Insert2(center, AABB(aabb.first, aabb.second));
	}
	std::cout << timer.getElapsedTime().asMicroseconds() / 1000.0 << " ms." << std::endl;
}

void Map::GeneratePolygons()
{
	sf::Clock timer;

	GeneratePoints();
	std::cout << "Point placement: " << timer.getElapsedTime().asMicroseconds() / 1000.0 << " ms." << std::endl;
	timer.restart();

	Triangulate(m_points);
	std::cout << "Triangulation: " << timer.getElapsedTime().asMicroseconds() / 1000.0 << " ms." << std::endl;
	timer.restart();

	FinishInfo();
	std::cout << "Finishing touches: " << timer.getElapsedTime().asMicroseconds() / 1000.0 << " ms." << std::endl;
}

void Map::GenerateLand()
{
	m_noiseMap = new noise::module::Perlin();

	for (auto corner : m_corners)
	{
		if (!corner->IsInsideBoundingBox(m_mapWidth, m_mapHeight))
		{
			corner->m_border = true;
			corner->m_ocean = true;
			corner->m_water = true;
		}
	}

	for (auto corner : m_corners)
	{
		corner->m_water = !IsIsland(corner->m_position);
	}
}

std::vector<Edge*> Map::GetEdges() const
{
	return m_edges;
}

std::vector<Corner*> Map::GetCorners() const
{
	return m_corners;
}

std::vector<Center*> Map::GetCenters() const
{
	return m_centers;
}

Center* Map::GetCenterAt(Vector2 pos)
{
	Center* center = nullptr;
	std::vector<Center*> auxCenters = m_centersQuadTree.QueryRange(pos);

	if (auxCenters.size() > 0)
	{
		double minDist = Vector2(auxCenters[0]->m_position, pos).Length();
		center = auxCenters[0];

		for (auto auxCenter : auxCenters)
		{
			double newDist = Vector2(auxCenter->m_position, pos).Length();
			if (newDist < minDist)
			{
				minDist = newDist;
				center = auxCenter;
			}
		}
	}

	return center;
}

bool Map::IsIsland(Vector2 position)
{
	double waterThreshold = 0.075;

	if (position.x < m_mapWidth * waterThreshold || position.y < m_mapHeight * waterThreshold ||
		position.x > m_mapWidth * (1 - waterThreshold) || position.y > m_mapHeight * (1 - waterThreshold))
	{
		return false;
	}

	Vector2 centerPos = Vector2(m_mapWidth / 2.0, m_mapHeight / 2.0);
	position -= centerPos;

	double xCoord = (position.x / m_mapWidth) * 4;
	double yCoord = (position.y / m_mapHeight) * 4;
	double noiseVal = m_noiseMap->GetValue(xCoord, yCoord, m_zCoord);

	position /= std::min(m_mapWidth, m_mapHeight);
	double radius = position.Length();

	double factor = radius - 0.5;

	return noiseVal >= 0.3 * radius + factor;
}

void Map::CalculateDownslopes()
{
	for (auto c : m_corners)
	{
		Corner* d = c;
		for (auto q : c->m_corners)
		{
			if (q->m_elevation < d->m_elevation)
			{
				d = q;
			}
		}

		c->m_downslope = d;
	}
}

void Map::GenerateRivers()
{
	std::mt19937 mt_rand(HashString(m_seed));
	int numRivers = m_centers.size() / 3;

	for (int i = 0; i < numRivers; ++i)
	{
		Corner* q = m_corners[mt_rand() % m_corners.size()];

		if (q->m_ocean || q->m_elevation < 0.3 || q->m_elevation > 0.9)
		{
			continue;
		}

		while (!q->m_coast)
		{
			if (q == q->m_downslope)
			{
				break;
			}

			Edge* e = q->GetEdgeWith(q->m_downslope);
			e->m_riverVolume += 1;
			q->m_riverVolume += 1;
			q->m_downslope->m_riverVolume += 1;
			q = q->m_downslope;
		}
	}
}

void Map::AssignOceanCoastLand()
{
	std::queue<Center*> centersQueue;

	for (auto c : m_centers)
	{
		int adjacentWater = 0;
		
		for (auto q : c->m_centers)
		{
			if (q->m_border)
			{
				c->m_border = true;
				c->m_ocean = true;
				q->m_water = true;
				centersQueue.push(c);
			}

			if (q->m_water)
			{
				adjacentWater++;
			}
		}

		c->m_water = (c->m_ocean || adjacentWater >= c->m_corners.size() * 0.5);
	}

	while (!centersQueue.empty())
	{
		Center* c = centersQueue.front();
		centersQueue.pop();

		for (auto r : c->m_centers)
		{
			if (r->m_water && !r->m_ocean)
			{
				r->m_ocean = true;
				centersQueue.push(r);
			}
		}
	}

	for (auto p : m_centers)
	{
		int numOcean = 0;
		int numLand = 0;

		for (auto q : p->m_centers)
		{
			numOcean += static_cast<int>(q->m_ocean);
			numLand += static_cast<int>(!q->m_water);
		}

		p->m_coast = numLand > 0 && numOcean > 0;
	}

	for (auto c : m_corners)
	{
		int adjOcean = 0;
		int adjLand = 0;

		for (auto p : c->m_centers)
		{
			adjOcean += static_cast<int>(p->m_ocean);
			adjLand += static_cast<int>(!p->m_water);
		}

		c->m_ocean = adjOcean == c->m_centers.size();
		c->m_coast = adjLand > 0 && adjOcean > 0;
		c->m_water = c->m_border || (adjLand != c->m_centers.size() && !c->m_coast);
	}
}

void RedistributeElevations();
void AssignCornerElevations();
void AssignPolygonElevations();
void RedistributeMoisture();
void AssignCornerMoisture();
void AssignPolygonMoisture();
void AssignBiomes();

void GeneratePoints();
void Triangulate(std::vector<DelaunayTriangulation::Vertex> points);
void FinishInfo();
void AddCenter(Center* c);
Center* GetCenter(Vector2 position);
void OrderPoints(std::vector<Corner*>& corners);

std::vector<Corner*> GetLandCorners();
std::vector<Corner*> GetLakeCorners();
void LloydRelaxation();
std::string CreateSeed(int length);

static unsigned int HashString(std::string seed);