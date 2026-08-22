// SpatialHashGrid.cpp
// ═══════════════════════════════════════════════════════════════════════════
//  Implementación del grid de hash espacial 2D para búsquedas de vecinos O(1).
// ═══════════════════════════════════════════════════════════════════════════

#include "AI/Crowd/SpatialHashGrid.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Init: inicializa el grid con un tamaño de celda y capacidad estimada.
// ─────────────────────────────────────────────────────────────────────────────
void FSpatialHashGrid::Init(float InCellSize, int32 InMaxAgents)
{
	CellSize = FMath::Max(InCellSize, 1.f);
	InvCellSize = 1.f / CellSize;

	// Dimensionar el grid para cubrir ~100m x 100m a la resolución dada.
	WorldGridSize = CellSize * 64.f;
	GridDim = FMath::Max(1, FMath::CeilToInt(WorldGridSize / CellSize));
	WorldGridSize = GridDim * CellSize;

	Cells.Empty(GridDim * GridDim);
	AgentToCell.Empty(InMaxAgents);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Clear: elimina todos los agentes del grid pero mantiene la memoria.
// ─────────────────────────────────────────────────────────────────────────────
void FSpatialHashGrid::Clear()
{
	for (auto& Pair : Cells)
	{
		Pair.Value.Reset();
	}
	AgentToCell.Reset();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Insert: coloca un agente en la celda correspondiente.
// ─────────────────────────────────────────────────────────────────────────────
void FSpatialHashGrid::Insert(int32 AgentIndex, const FVector& Position)
{
	const int32 Key = PositionToCellKey(Position);
	Cells.FindOrAdd(Key).Add(AgentIndex);
	AgentToCell.Add(AgentIndex, Key);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Remove: elimina un agente de su celda actual.
// ─────────────────────────────────────────────────────────────────────────────
void FSpatialHashGrid::Remove(int32 AgentIndex)
{
	const int32* FoundKey = AgentToCell.Find(AgentIndex);
	if (FoundKey == nullptr)
	{
		return;
	}

	const int32 Key = *FoundKey;
	TArray<int32>* Bucket = Cells.Find(Key);
	if (Bucket != nullptr)
	{
		Bucket->Remove(AgentIndex);
	}
	AgentToCell.Remove(AgentIndex);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Move: reasigna un agente si cambió de celda entre old y new.
// ─────────────────────────────────────────────────────────────────────────────
void FSpatialHashGrid::Move(int32 AgentIndex, const FVector& OldPosition, const FVector& NewPosition)
{
	const int32 OldKey = PositionToCellKey(OldPosition);
	const int32 NewKey = PositionToCellKey(NewPosition);

	if (OldKey == NewKey)
	{
		// Sigue en la misma celda, no hacer nada.
		return;
	}

	// Eliminar de la celda antigua.
	TArray<int32>* OldBucket = Cells.Find(OldKey);
	if (OldBucket != nullptr)
	{
		OldBucket->Remove(AgentIndex);
	}

	// Insertar en la celda nueva.
	Cells.FindOrAdd(NewKey).Add(AgentIndex);
	AgentToCell.Add(AgentIndex, NewKey);
}

// ─────────────────────────────────────────────────────────────────────────────
//  QueryNeighbors: busca en la celda del agente y las 8 adyacentes.
//  Port directo del loop FlockingJob del SistemaMultitud de Unity.
// ─────────────────────────────────────────────────────────────────────────────
void FSpatialHashGrid::QueryNeighbors(TArray<int32>& OutNeighbors, const FVector& Position, float Radius) const
{
	OutNeighbors.Reset();

	int32 Cx0, Cz0;
	PositionToCellCoords(Position - FVector(Radius, Radius, 0.f), Cx0, Cz0);

	int32 Cx1, Cz1;
	PositionToCellCoords(Position + FVector(Radius, Radius, 0.f), Cx1, Cz1);

	// Clamp a los límites del grid.
	Cx0 = FMath::Clamp(Cx0, 0, GridDim - 1);
	Cx1 = FMath::Clamp(Cx1, 0, GridDim - 1);
	Cz0 = FMath::Clamp(Cz0, 0, GridDim - 1);
	Cz1 = FMath::Clamp(Cz1, 0, GridDim - 1);

	const float RadiusSq = Radius * Radius;

	for (int32 Cz = Cz0; Cz <= Cz1; ++Cz)
	{
		for (int32 Cx = Cx0; Cx <= Cx1; ++Cx)
		{
			const int32 Key = Cz * GridDim + Cx;
			const TArray<int32>* Bucket = Cells.Find(Key);
			if (Bucket == nullptr)
			{
				continue;
			}

			for (int32 Idx : *Bucket)
			{
				OutNeighbors.Add(Idx);
			}
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  GetCellCount: devuelve cuántos agentes hay en la celda de una posición.
// ─────────────────────────────────────────────────────────────────────────────
int32 FSpatialHashGrid::GetCellCount(const FVector& Position) const
{
	const int32 Key = PositionToCellKey(Position);
	const TArray<int32>* Bucket = Cells.Find(Key);
	return Bucket ? Bucket->Num() : 0;
}

// ─────────────────────────────────────────────────────────────────────────────
//  PositionToCellKey: convierte una posición world a una key de celda.
// ─────────────────────────────────────────────────────────────────────────────
int32 FSpatialHashGrid::PositionToCellKey(const FVector& Position) const
{
	int32 Cx, Cz;
	PositionToCellCoords(Position, Cx, Cz);
	return Cz * GridDim + Cx;
}

// ─────────────────────────────────────────────────────────────────────────────
//  PositionToCellCoords: extrae coordenadas (cx, cz) de una posición world.
// ─────────────────────────────────────────────────────────────────────────────
void FSpatialHashGrid::PositionToCellCoords(const FVector& Position, int32& OutCx, int32& OutCz) const
{
	const float RelX = Position.X - GridOrigin.X;
	const float RelZ = Position.Y - GridOrigin.Y;

	OutCx = FMath::Clamp(FMath::FloorToInt(RelX * InvCellSize), 0, GridDim - 1);
	OutCz = FMath::Clamp(FMath::FloorToInt(RelZ * InvCellSize), 0, GridDim - 1);
}

// ─────────────────────────────────────────────────────────────────────────────
//  CellKeyToCoords: convierte una key a coordenadas (cx, cz).
// ─────────────────────────────────────────────────────────────────────────────
void FSpatialHashGrid::CellKeyToCoords(int32 Key, int32& OutCx, int32& OutCz) const
{
	OutCx = Key % GridDim;
	OutCz = Key / GridDim;
}
