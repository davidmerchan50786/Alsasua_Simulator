#include "World/AlsasuaStreetGraph.h"
#include "Engine/World.h"
#include "GeoDataAlsasua.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

void UAlsasuaStreetGraph::BuildFromRoadsJson(const FString& JsonPath, UWorld* World)
{
	Nodes.Empty();
	Edges.Empty();

	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("StreetGraph: World nulo, no se puede convertir coordenadas"));
		return;
	}

	FString JsonStr;
	if (!FFileHelper::LoadFileToString(JsonStr, *JsonPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("StreetGraph: No se pudo cargar %s"), *JsonPath);
		return;
	}

	TArray<TSharedPtr<FJsonValue>> RoadsArr;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
	if (!FJsonSerializer::Deserialize(Reader, RoadsArr))
	{
		UE_LOG(LogTemp, Warning, TEXT("StreetGraph: Error parseando %s"), *JsonPath);
		return;
	}

	for (const auto& RoadVal : RoadsArr)
	{
		const TSharedPtr<FJsonObject>& Road = RoadVal->AsObject();
		if (!Road) continue;

		FStreetEdge Edge;
		Road->TryGetStringField(TEXT("name"), Edge.StreetName);
		Road->TryGetStringField(TEXT("type"), Edge.StreetType);
		Road->TryGetBoolField(TEXT("oneway"), Edge.bOneWay);

		const TArray<TSharedPtr<FJsonValue>>* PointsArr;
		if (!Road->TryGetArrayField(TEXT("points"), PointsArr)) continue;
		if (PointsArr->Num() < 2) continue;

		TArray<FVector> Puntos;
		Puntos.Reserve(PointsArr->Num());
		for (const auto& PtVal : *PointsArr)
		{
			const TSharedPtr<FJsonObject>& Pt = PtVal->AsObject();
			if (!Pt) continue;
			Puntos.Add(UAlsasuaGeoData::RelLocalASueloUE5(World,
				FVector(Pt->GetNumberField(TEXT("x")), 0.0f, Pt->GetNumberField(TEXT("z")))));
		}
		if (Puntos.Num() < 2) continue;

		Edge.NodeA = FindOrCreateNode(Puntos[0]);
		Edge.NodeB = FindOrCreateNode(Puntos.Last());
		for (int32 i = 1; i < Puntos.Num() - 1; ++i)
			Edge.WayPoints.Add(Puntos[i]);

		Edge.Length = 0.0f;
		for (int32 i = 0; i < Puntos.Num() - 1; ++i)
			Edge.Length += FVector::Dist(Puntos[i], Puntos[i + 1]);

		const int32 EdgeIdx = Edges.Add(Edge);
		Nodes[Edges[EdgeIdx].NodeA].ConnectedEdges.Add(EdgeIdx);
		Nodes[Edges[EdgeIdx].NodeB].ConnectedEdges.Add(EdgeIdx);
	}

	UE_LOG(LogTemp, Log, TEXT("StreetGraph: %d nodos, %d calles desde %s"),
		Nodes.Num(), Edges.Num(), *JsonPath);
}

// ponytail: scan lineal O(nodos) por llamada (O(n^2) en el build total);
// con ~500 calles de Alsasua son milisegundos. Si crece a decenas de miles
// de nodos, cambiar por un TMap de celda→nodos.
int32 UAlsasuaStreetGraph::FindOrCreateNode(const FVector& Pos, float SnapThreshold)
{
	const float UmbralSq = SnapThreshold * SnapThreshold;
	int32 MejorIdx = INDEX_NONE;
	float MejorDistSq = UmbralSq;
	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		const float DistSq = FVector::DistSquared(Nodes[i].Position, Pos);
		if (DistSq <= MejorDistSq)
		{
			MejorDistSq = DistSq;
			MejorIdx = i;
		}
	}
	if (MejorIdx != INDEX_NONE) return MejorIdx;

	FStreetNode Nodo;
	Nodo.Position = Pos;
	return Nodes.Add(Nodo);
}

TArray<int32> UAlsasuaStreetGraph::FindPath(int32 StartNode, int32 EndNode) const
{
	TArray<int32> Camino;
	if (!Nodes.IsValidIndex(StartNode) || !Nodes.IsValidIndex(EndNode)) return Camino;
	if (StartNode == EndNode)
	{
		Camino.Add(StartNode);
		return Camino;
	}

	const int32 NumNodos = Nodes.Num();
	constexpr float Inf = TNumericLimits<float>::Max();

	TArray<float> GScore;
	GScore.Init(Inf, NumNodos);
	TArray<int32> CameFrom;
	CameFrom.Init(INDEX_NONE, NumNodos);
	TArray<bool> Cerrado;
	Cerrado.Init(false, NumNodos);

	auto Heuristica = [&](int32 Idx)
	{
		return FVector::Distance(Nodes[Idx].Position, Nodes[EndNode].Position);
	};

	// Open set como array con extracción lineal del mínimo f: grafo pequeño,
	// llamadas esporádicas; montículo si algún día hay cientos de rutas/frame.
	TArray<int32> Abierta;
	GScore[StartNode] = 0.0f;
	Abierta.Add(StartNode);

	while (Abierta.Num() > 0)
	{
		int32 MejorPos = 0;
		float MejorF = Inf;
		for (int32 i = 0; i < Abierta.Num(); ++i)
		{
			const float F = GScore[Abierta[i]] + Heuristica(Abierta[i]);
			if (F < MejorF)
			{
				MejorF = F;
				MejorPos = i;
			}
		}

		const int32 Actual = Abierta[MejorPos];
		Abierta.RemoveAtSwap(MejorPos);

		if (Actual == EndNode)
		{
			for (int32 It = EndNode; It != INDEX_NONE; It = CameFrom[It])
			{
				Camino.Insert(It, 0);
				if (It == StartNode) break;
			}
			if (Camino.Num() == 0 || Camino[0] != StartNode) Camino.Empty();
			return Camino;
		}

		if (Cerrado[Actual]) continue;
		Cerrado[Actual] = true;

		for (const int32 EdgeIdx : Nodes[Actual].ConnectedEdges)
		{
			const FStreetEdge& Arista = Edges[EdgeIdx];
			const bool bHaciaB = (Arista.NodeA == Actual);
			if (Arista.bOneWay && !bHaciaB) continue;

			const int32 Vecino = bHaciaB ? Arista.NodeB : Arista.NodeA;
			if (Cerrado[Vecino]) continue;

			const float Tentativo = GScore[Actual] + Arista.Length;
			if (Tentativo < GScore[Vecino])
			{
				GScore[Vecino] = Tentativo;
				CameFrom[Vecino] = Actual;
				if (!Abierta.Contains(Vecino)) Abierta.Add(Vecino);
			}
		}
	}

	return Camino;
}

const FStreetEdge* UAlsasuaStreetGraph::EncontrarEdge(int32 A, int32 B) const
{
	if (!Nodes.IsValidIndex(A)) return nullptr;
	for (const int32 EdgeIdx : Nodes[A].ConnectedEdges)
	{
		const FStreetEdge& Arista = Edges[EdgeIdx];
		if ((Arista.NodeA == A && Arista.NodeB == B) ||
		    (Arista.NodeA == B && Arista.NodeB == A))
		{
			return &Arista;
		}
	}
	return nullptr;
}

float UAlsasuaStreetGraph::EstimateRouteDistance(int32 A, int32 B) const
{
	const TArray<int32> Camino = FindPath(A, B);
	if (Camino.Num() < 2) return -1.0f;

	float Total = 0.0f;
	for (int32 i = 0; i < Camino.Num() - 1; ++i)
	{
		if (const FStreetEdge* Arista = EncontrarEdge(Camino[i], Camino[i + 1]))
			Total += Arista->Length;
		else // no debería pasar con un grafo bien construido
			Total += FVector::Distance(Nodes[Camino[i]].Position, Nodes[Camino[i + 1]].Position);
	}
	return Total;
}

TArray<FVector> UAlsasuaStreetGraph::GetRoutePoints(const TArray<int32>& PathNodes) const
{
	TArray<FVector> Puntos;
	if (PathNodes.Num() == 0 || !Nodes.IsValidIndex(PathNodes[0])) return Puntos;

	Puntos.Add(Nodes[PathNodes[0]].Position);
	for (int32 i = 0; i < PathNodes.Num() - 1; ++i)
	{
		if (const FStreetEdge* Arista = EncontrarEdge(PathNodes[i], PathNodes[i + 1]))
			Puntos.Append(Arista->WayPoints);
		if (Nodes.IsValidIndex(PathNodes[i + 1]))
			Puntos.Add(Nodes[PathNodes[i + 1]].Position);
	}
	return Puntos;
}

int32 UAlsasuaStreetGraph::GetNearestNode(const FVector& Pos) const
{
	int32 MejorIdx = INDEX_NONE;
	float MejorDistSq = TNumericLimits<float>::Max();
	for (int32 i = 0; i < Nodes.Num(); ++i)
	{
		const float DistSq = FVector::DistSquared(Nodes[i].Position, Pos);
		if (DistSq < MejorDistSq)
		{
			MejorDistSq = DistSq;
			MejorIdx = i;
		}
	}
	return MejorIdx;
}
