// FachadasEdificios.shader
// Shader URP para fachadas de edificios en Cesium OSM Buildings.
//
// USA MAPEO TRIPLANAR EN ESPACIO DE MUNDO:
//   La textura se proyecta usando la posición en world-space, no las UV del glTF.
//   Esto hace que el número de ventanas escale con el tamaño real del edificio:
//   una pared de 12 m muestra 3 ventanas si _TileMetros = 4.
//   Las UV del glTF van siempre 0→1 por cara, independientemente del tamaño,
//   por lo que NO sirven para tiling realista en edificios de distinto tamaño.
//
// PASES URP INCLUIDOS:
//   ForwardLit    — renderizado principal con Lambert + ambiente
//   ShadowCaster  — genera sombras correctas sobre el terreno
//   DepthOnly     — necesario para SSAO y efectos de profundidad en URP

Shader "Alsasua/FachadasEdificios"
{
    Properties
    {
        _WallTex    ("Textura Fachada (paredes)", 2D)     = "white"  {}
        _RoofTex    ("Textura Tejado",            2D)     = "gray"   {}
        _TileMetros ("Metros por tile de textura", Float) = 4.0
        _WallTint   ("Color pared",   Color)              = (0.92, 0.87, 0.78, 1.0)
        _RoofTint   ("Color tejado",  Color)              = (0.55, 0.50, 0.45, 1.0)
        _Ambient    ("Luz ambiente",  Range(0.0, 1.0))    = 0.40
        _RoofBlend  ("Fuerza tejado", Range(0.0, 1.0))    = 0.85
    }

    SubShader
    {
        Tags
        {
            "RenderType"      = "Opaque"
            "RenderPipeline"  = "UniversalPipeline"
            "Queue"           = "Geometry"
        }
        LOD 200

        // ──────────────────────────────────────────────────────────────
        //  PASE PRINCIPAL: ForwardLit
        // ──────────────────────────────────────────────────────────────
        Pass
        {
            Name "ForwardLit"
            Tags { "LightMode" = "UniversalForward" }

            Cull Back
            ZWrite On

            HLSLPROGRAM
            #pragma vertex   vert
            #pragma fragment frag

            // Instancing y variantes de sombras
            #pragma multi_compile_instancing
            #pragma multi_compile _ _MAIN_LIGHT_SHADOWS _MAIN_LIGHT_SHADOWS_CASCADE
            #pragma multi_compile _ _SHADOWS_SOFT

            #include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/Core.hlsl"
            #include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/Lighting.hlsl"

            TEXTURE2D(_WallTex);  SAMPLER(sampler_WallTex);
            TEXTURE2D(_RoofTex);  SAMPLER(sampler_RoofTex);

            CBUFFER_START(UnityPerMaterial)
                float4 _WallTex_ST;
                float4 _RoofTex_ST;
                float4 _WallTint;
                float4 _RoofTint;
                float  _TileMetros;
                float  _Ambient;
                float  _RoofBlend;
            CBUFFER_END

            struct Attributes
            {
                float4 positionOS : POSITION;
                float3 normalOS   : NORMAL;
                UNITY_VERTEX_INPUT_INSTANCE_ID
            };

            struct Varyings
            {
                float4 positionCS : SV_POSITION;
                float3 positionWS : TEXCOORD0;
                float3 normalWS   : TEXCOORD1;
                float4 shadowCoord: TEXCOORD2;
                UNITY_VERTEX_OUTPUT_STEREO
            };

            Varyings vert(Attributes v)
            {
                Varyings o;
                UNITY_SETUP_INSTANCE_ID(v);
                UNITY_INITIALIZE_VERTEX_OUTPUT_STEREO(o);

                VertexPositionInputs posInputs = GetVertexPositionInputs(v.positionOS.xyz);
                VertexNormalInputs   nmInputs  = GetVertexNormalInputs(v.normalOS);

                o.positionCS = posInputs.positionCS;
                o.positionWS = posInputs.positionWS;
                o.normalWS   = nmInputs.normalWS;
                o.shadowCoord = GetShadowCoord(posInputs);
                return o;
            }

            half4 frag(Varyings i) : SV_Target
            {
                float3 N = normalize(i.normalWS);
                float  scale = 1.0 / max(_TileMetros, 0.01);

                // ── Triplanar blend ─────────────────────────────────────
                // Usar pow(6) para transiciones nítidas entre cara y tejado
                float3 blend = pow(abs(N), 6.0);
                blend /= (blend.x + blend.y + blend.z + 1e-5);

                // UV en world-space: cada eje usa 2 de las 3 coords del mundo
                float2 uvX = i.positionWS.zy * scale;   // cara X (este/oeste)
                float2 uvY = i.positionWS.xz * scale;   // cara Y (tejado/suelo)
                float2 uvZ = i.positionWS.xy * scale;   // cara Z (norte/sur)

                // Muestras de pared y tejado
                half4 wallX = SAMPLE_TEXTURE2D(_WallTex, sampler_WallTex, uvX) * _WallTint;
                half4 roofY = SAMPLE_TEXTURE2D(_RoofTex, sampler_RoofTex, uvY) * _RoofTint;
                half4 wallZ = SAMPLE_TEXTURE2D(_WallTex, sampler_WallTex, uvZ) * _WallTint;

                // Mezcla: paredes en X y Z, tejado en Y ponderado por _RoofBlend
                float roofW = blend.y * _RoofBlend;
                float wallW = blend.x + blend.z + blend.y * (1.0 - _RoofBlend);
                half4 col   = (wallX * blend.x + wallZ * blend.z + roofY * roofW)
                              / max(wallW + roofW, 1e-5);

                // ── Iluminación Lambert + sombras + ambiente ─────────────
                Light  mainLight = GetMainLight(i.shadowCoord);
                float  NdotL     = saturate(dot(N, mainLight.direction));
                float  shadow    = mainLight.shadowAttenuation;
                half3  diffuse   = col.rgb * mainLight.color * (NdotL * shadow + _Ambient);

                return half4(diffuse, 1.0);
            }
            ENDHLSL
        }

        // ──────────────────────────────────────────────────────────────
        //  PASE SOMBRAS: ShadowCaster
        //  Necesario para que los edificios proyecten sombras sobre el terreno.
        // ──────────────────────────────────────────────────────────────
        Pass
        {
            Name "ShadowCaster"
            Tags { "LightMode" = "ShadowCaster" }

            ZWrite On
            ZTest  LEqual
            ColorMask 0
            Cull Back

            HLSLPROGRAM
            #pragma vertex   shadowVert
            #pragma fragment shadowFrag
            #pragma multi_compile_instancing

            #include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/Core.hlsl"
            #include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/Shadows.hlsl"

            // _LightDirection lo provee el pipeline de sombras de URP
            float3 _LightDirection;

            struct ShadowAttribs
            {
                float4 positionOS : POSITION;
                float3 normalOS   : NORMAL;
                UNITY_VERTEX_INPUT_INSTANCE_ID
            };

            struct ShadowVaryings
            {
                float4 positionCS : SV_POSITION;
                UNITY_VERTEX_OUTPUT_STEREO
            };

            ShadowVaryings shadowVert(ShadowAttribs v)
            {
                ShadowVaryings o;
                UNITY_SETUP_INSTANCE_ID(v);
                UNITY_INITIALIZE_VERTEX_OUTPUT_STEREO(o);

                float3 posWS  = TransformObjectToWorld(v.positionOS.xyz);
                float3 normWS = TransformObjectToWorldNormal(v.normalOS);
                float4 posCS  = TransformWorldToHClip(ApplyShadowBias(posWS, normWS, _LightDirection));

                // Clamp depth para evitar artefactos en luces direccionales
                #if UNITY_REVERSED_Z
                    posCS.z = min(posCS.z, UNITY_NEAR_CLIP_VALUE);
                #else
                    posCS.z = max(posCS.z, UNITY_NEAR_CLIP_VALUE);
                #endif

                o.positionCS = posCS;
                return o;
            }

            half4 shadowFrag(ShadowVaryings i) : SV_Target { return 0; }
            ENDHLSL
        }

        // ──────────────────────────────────────────────────────────────
        //  PASE PROFUNDIDAD: DepthOnly
        //  Necesario para SSAO, DoF y otros efectos de post-procesado URP.
        // ──────────────────────────────────────────────────────────────
        Pass
        {
            Name "DepthOnly"
            Tags { "LightMode" = "DepthOnly" }

            ZWrite On
            ColorMask R
            Cull Back

            HLSLPROGRAM
            #pragma vertex   depthVert
            #pragma fragment depthFrag
            #pragma multi_compile_instancing

            #include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/Core.hlsl"

            struct DepthAttribs  { float4 positionOS : POSITION; UNITY_VERTEX_INPUT_INSTANCE_ID };
            struct DepthVaryings { float4 positionCS : SV_POSITION; UNITY_VERTEX_OUTPUT_STEREO  };

            DepthVaryings depthVert(DepthAttribs v)
            {
                DepthVaryings o;
                UNITY_SETUP_INSTANCE_ID(v);
                UNITY_INITIALIZE_VERTEX_OUTPUT_STEREO(o);
                o.positionCS = TransformObjectToHClip(v.positionOS.xyz);
                return o;
            }

            half depthFrag(DepthVaryings i) : SV_Target { return 0; }
            ENDHLSL
        }
    }

    FallBack "Hidden/Universal Render Pipeline/FallbackError"
}
