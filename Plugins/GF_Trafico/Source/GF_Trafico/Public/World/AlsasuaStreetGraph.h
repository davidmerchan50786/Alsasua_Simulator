#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AlsasuaStreetGraph.generated.h"

/** Nodo del grafo de calles: posición en espacio UE5 y aristas que lo tocan. */
USTRUCT()
struct FStreetNode
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Position = FVector::ZeroVector;

	UPROPERTY()
	TArray<int32> ConnectedEdges;
};

/** Arista del grafo: una calle entre dos nodos, con sus puntos intermedios. */
USTRUCT()
struct FStreetEdge
{
	GENERATED_BODY()

	UPROPERTY()
	int32 NodeA = INDEX_NONE;

	UPROPERTY()
	int32 NodeB = INDEX_NONE;

	UPROPERTY()
	FString StreetName;

	UPROPERTY()
	FString StreetType;

	UPROPERTY()
	bool bOneWay = false;

	/** km/h */
	UPROPERTY()
	float SpeedLimit = 50.0f;

	UPROPERTY()
	TArray<FVector> WayPoints;

	/** cm, suma de tramos nodo→waypoint→nodo */
	UPROPERTY()
	float Length = 0.0f;
};

/**
 * Grafo de calles construido desde Content/Datos/roads_unity.json para rutas.
 *
 * Cada polilínea del JSON se convierte en una arista entre el nodo inicial y el
 * final (los puntos intermedios quedan como WayPoints). Los extremos que caen
 * a menos de SnapThreshold se funden en un mismo nodo, y así aparecen las
 * intersecciones sin información extra de conectividad.
 */
UCLASS()
class GF_TRAFICO_API UAlsasuaStreetGraph : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TArray<FStreetNode> Nodes;

	UPROPERTY()
	TArray<FStreetEdge> Edges;

	/** Construye el grafo desde un JSON tipo roads_unity.json. Vacía lo anterior. */
	void BuildFromRoadsJson(const FString& JsonPath, UWorld* World);

	/** Devuelve el índice del nodo en Pos o a menos de SnapThreshold; si no, crea uno nuevo. */
	int32 FindOrCreateNode(const FVector& Pos, float SnapThreshold = 150.0f);

	/** A* con heurística euclídea. Camino de índices de nodo inicio→fin; vacío si no hay ruta. */
	TArray<int32> FindPath(int32 StartNode, int32 EndNode) const;

	/** Longitud total de la ruta A→B en cm; -1 si no hay camino. */
	float EstimateRouteDistance(int32 A, int32 B) const;

	/** Polilínea completa (posiciones de nodo + waypoints) recorriendo PathNodes. */
	TArray<FVector> GetRoutePoints(const TArray<int32>& PathNodes) const;

	/** Nodo más cercano a Pos; INDEX_NONE si el grafo está vacío. */
	int32 GetNearestNode(const FVector& Pos) const;

private:
	const FStreetEdge* EncontrarEdge(int32 A, int32 B) const;
};
