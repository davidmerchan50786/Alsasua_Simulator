// SpatialHashGrid.h
// ═══════════════════════════════════════════════════════════════════════════
//  Grid de hash espacial 2D para búsquedas O(1) de vecinos.
//  Port de la estructura NativeArray del SistemaMultitud de Unity a UE 5.4.
//
//  Diseño:
//   · Celdas de tamaño configurable (CellSize).
//   · Cada celda contiene un TArray<int32> con los índices de agentes.
//   · Insert/Remove/Move reasignan agentes entre celdas.
//   · QueryNeighbors busca en la celda del agente y las 8 adyacentes.
// ═══════════════════════════════════════════════════════════════════════════

#pragma once

#include "CoreMinimal.h"
#include "SpatialHashGrid.generated.h"

/**
 * Grid de hash espacial 2D optimizado para O(1) de inserción, eliminación
 * y búsqueda de vecinos en radio. Inspirado en la NativeArray flat-grid del
 * SistemaMultitud original de Unity.
 */
USTRUCT()
struct FSpatialHashGrid
{
	GENERATED_BODY()

	FSpatialHashGrid() = default;

	/**
	 * Inicializa (o reinicializa) el grid con un tamaño dado.
	 * @param InCellSize  Tamaño de cada celda (cm). Define la resolución.
	 * @param InMaxAgents Capacidad máxima de agentes esperados (pre-asigna memoria).
	 */
	void Init(float InCellSize, int32 InMaxAgents);

	/** Limpia todos los buckets (mantiene la memoria reservada). */
	void Clear();

	/**
	 * Inserta un agente en el grid en la posición dada.
	 * @param AgentIndex  Índice del agente en el array externo.
	 * @param Position    Posición world del agente.
	 */
	void Insert(int32 AgentIndex, const FVector& Position);

	/**
	 * Elimina un agente del grid.
	 * @param AgentIndex  Índice del agente a eliminar.
	 */
	void Remove(int32 AgentIndex);

	/**
	 * Mueve un agente a una nueva celda (si cambió de celda).
	 * Más eficiente que Remove + Insert.
	 * @param AgentIndex  Índice del agente.
	 * @param OldPosition Antigua posición world.
	 * @param NewPosition Nueva posición world.
	 */
	void Move(int32 AgentIndex, const FVector& OldPosition, const FVector& NewPosition);

	/**
	 * Consulta vecinos de un agente dentro de un radio dado.
	 * Busca en la celda del agente y las 8 adyacentes.
	 * @param OutNeighbors Array de salida con los índices de vecinos (NO incluye el propio agente).
	 * @param Position     Posición del agente consultante.
	 * @param Radius       Radio de búsqueda.
	 */
	void QueryNeighbors(TArray<int32>& OutNeighbors, const FVector& Position, float Radius) const;

	/** Devuelve el número de agentes en una celda dada (para debugging). */
	int32 GetCellCount(const FVector& Position) const;

	/** Tamaño de celda actual (cm). */
	float CellSize = 150.f;

	/** Inverso del tamaño de celda (precomputado). */
	float InvCellSize = 1.f / 150.f;

private:
	/** Dimensión del grid (NxN). */
	int32 GridDim = 64;

	/** Origen del grid (esquina inferior-izquierda en XY, plano horizontal). */
	FVector GridOrigin = FVector::ZeroVector;

	/** Tamaño total del grid en unidades world. */
	float WorldGridSize = 0.f;

	/** Mapa de celda → lista de agentes. Key = cy * GridDim + cx. */
	TMap<int32, TArray<int32>> Cells;

	/** Mapa de agente → celda key actual (para Move/Remove rápido). */
	TMap<int32, int32> AgentToCell;

	/** Convierte una posición world a una key de celda. */
	int32 PositionToCellKey(const FVector& Position) const;

	/** Convierte una key de celda a coordenadas (cx, cz). */
	void CellKeyToCoords(int32 Key, int32& OutCx, int32& OutCz) const;

	/** Extrae coordenadas de celda de una posición world. */
	void PositionToCellCoords(const FVector& Position, int32& OutCx, int32& OutCz) const;
};
