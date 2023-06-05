#include "reverse.h"
#include "rat.h"
Memory* KmDrv = nullptr;
#include "d3d9_x.h"
#include "xor.hpp"
#include <dwmapi.h>
#include <vector>
#include "Keybind.h"
#include "offsets.h"
#include <fstream>
#include <filesystem>
#include <thread>
#include "imgui_animated.h"
#include "json.h"
#pragma comment(lib,"urlmon.lib") //for urldownload
bool fovcircle = true;
bool ShowMenu = true;
bool Esp = true;
bool Esp_box = true;
bool cornerbox = false;
bool skeleton = true;
bool Esp_info = true;
bool aimline = true;
bool headdot = true;
bool Esp_line = false;
bool Aimbot = true;
bool triggerbot = false;
bool memoryaimbot = false;
bool silent = false;
bool prediction = false;
bool streamproof = false;
ImFont* m_pFont;
float smooth = 3;
float memorysmoothing = 20;
static int VisDist = 270;
float AimFOV = 150;
static int aimkey;
static int hitbox;
static int aimkeypos = 3;
static int hitboxpos = 1;
bool PressingTrigger{ false };
DWORD_PTR Uworld;
DWORD_PTR LocalPawn;
DWORD_PTR PlayerState;
DWORD_PTR Localplayer;
DWORD_PTR Rootcomp;
DWORD_PTR PlayerController;
DWORD_PTR Persistentlevel;
uintptr_t PlayerCameraManager;
Vector3 localactorpos;

uint64_t TargetPawn;
int localplayerID;

RECT GameRect = { NULL };
D3DPRESENT_PARAMETERS d3dpp;

DWORD ScreenCenterX;
DWORD ScreenCenterY;
Vector3 LocalRelativeLocation; struct FBoxSphereBounds
{
	struct Vector3                                     Origin;                                                   // 0x0000(0x0018) (Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData)
	struct Vector3                                     BoxExtent;                                                // 0x0018(0x0018) (Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData)
	double                                             SphereRadius;                                             // 0x0030(0x0008) (Edit, BlueprintVisible, ZeroConstructor, SaveGame, IsPlainOldData)
};
static void xCreateWindow();
static void xInitD3d();
static void xMainLoop();
static void xShutdown();
static LRESULT CALLBACK WinProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static HWND Window = NULL;
IDirect3D9Ex* p_Object = NULL;
static LPDIRECT3DDEVICE9 D3dDevice = NULL;
static LPDIRECT3DVERTEXBUFFER9 TriBuf = NULL;

DWORD Menuthread(LPVOID in)
{
	while (1)
	{
		if (MouseController::GetAsyncKeyState(VK_INSERT) & 1) {
			ShowMenu = !ShowMenu;
		}
		Sleep(2);
	}
}

class GradientLine {
public:

	static bool Render(ImVec2 size)
	{
		static ImColor gradient_colors[] =
		{
			//https://txwes.libguides.com/c.php?g=978475&p=7075536

			//Red
			ImColor(255, 0, 0),
			//Yellow
			ImColor(255,255,0),
			//Lime
			ImColor(0, 255, 0),
			//Cyan / Aqua
			ImColor(0, 255, 255),
			//Blue
			ImColor(0, 0, 255),
			//Magenta / Fuchsia
			ImColor(255, 0, 255),
			//Red
			ImColor(255, 0, 0)
		};

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2      screen_pos = ImGui::GetCursorScreenPos();

		static int pos = 0;

		if (size.x - pos == 0)
			pos = 0;
		else
			pos++;

		for (int i = 0; i < 6; ++i)
		{
			ImVec2 item_spacing = ImGui::GetStyle().ItemSpacing;

			auto render = [&](int displacement)
			{
				draw_list->AddRectFilledMultiColor
				(
					ImVec2((screen_pos.x - item_spacing.x - displacement) + (i) * (size.x / 6), (screen_pos.y - item_spacing.y)),
					ImVec2((screen_pos.x - item_spacing.x + (item_spacing.x * 2) - displacement) + (i + 1) * (size.x / 6), (screen_pos.y - item_spacing.y) + (size.y)),

					//add one to color to create a skuffed blend
					gradient_colors[i], gradient_colors[i + 1], gradient_colors[i + 1], gradient_colors[i]
				);
			};

			render((pos)); render((pos - size.x));
		}
		return true;
	}
};


//pasta made by Zyko#5528
FTransform GetBoneIndex(DWORD_PTR mesh, int index)
{
	DWORD_PTR bonearray;
	bonearray = KmDrv->Rpm<DWORD_PTR>(mesh + OFFSETS::BoneArray);

	if (bonearray == NULL)
	{
		bonearray = KmDrv->Rpm<DWORD_PTR>(mesh + OFFSETS::BoneArray + 0x10);
	}
	return  KmDrv->Rpm<FTransform>(bonearray + (index * 0x60));
}

Vector3 GetBoneWithRotation(DWORD_PTR mesh, int id)
{
	FTransform bone = GetBoneIndex(mesh, id);
	FTransform ComponentToWorld = KmDrv->Rpm<FTransform>(mesh + OFFSETS::ComponetToWorld);

	D3DMATRIX Matrix;
	Matrix = MatrixMultiplication(bone.ToMatrixWithScale(), ComponentToWorld.ToMatrixWithScale());

	return Vector3(Matrix._41, Matrix._42, Matrix._43);
}
D3DXMATRIX Matrix(Vector3 rot, Vector3 origin = Vector3(0, 0, 0))
{
	float radPitch = (rot.x * float(M_PI) / 180.f);
	float radYaw = (rot.y * float(M_PI) / 180.f);
	float radRoll = (rot.z * float(M_PI) / 180.f);

	float SP = sinf(radPitch);
	float CP = cosf(radPitch);
	float SY = sinf(radYaw);
	float CY = cosf(radYaw);
	float SR = sinf(radRoll);
	float CR = cosf(radRoll);

	D3DMATRIX matrix;
	matrix.m[0][0] = CP * CY;
	matrix.m[0][1] = CP * SY;
	matrix.m[0][2] = SP;
	matrix.m[0][3] = 0.f;

	matrix.m[1][0] = SR * SP * CY - CR * SY;
	matrix.m[1][1] = SR * SP * SY + CR * CY;
	matrix.m[1][2] = -SR * CP;
	matrix.m[1][3] = 0.f;

	matrix.m[2][0] = -(CR * SP * CY + SR * SY);
	matrix.m[2][1] = CY * SR - CR * SP * SY;
	matrix.m[2][2] = CR * CP;
	matrix.m[2][3] = 0.f;

	matrix.m[3][0] = origin.x;
	matrix.m[3][1] = origin.y;
	matrix.m[3][2] = origin.z;
	matrix.m[3][3] = 1.f;

	return matrix;
}


double __fastcall Atan2(double a1, double a2)
{
	double result; // xmm0_8

	result = 0.0;
	if (a2 != 0.0 || a1 != 0.0)
		return atan2(a1, a2);
	return result;
}
double __fastcall FMod(double a1, double a2)
{
	if (fabs(a2) > 0.00000001)
		return fmod(a1, a2);
	else
		return 0.0;
}

double ClampAxis(double Angle)
{
	// returns Angle in the range (-360,360)
	Angle = FMod(Angle, (double)360.0);

	if (Angle < (double)0.0)
	{
		// shift to [0,360) range
		Angle += (double)360.0;
	}

	return Angle;
}

double NormalizeAxis(double Angle)
{
	// returns Angle in the range [0,360)
	Angle = ClampAxis(Angle);

	if (Angle > (double)180.0)
	{
		// shift to (-180,180]
		Angle -= (double)360.0;
	}

	return Angle;
}


typedef struct {
	float Pitch;
	float Yaw;
	float Roll;
} FRotator;

FRotator Rotator(FQuat* F)
{
	const double SingularityTest = F->z * F->x - F->w * F->y;
	const double YawY = 2.f * (F->w * F->z + F->x * F->y);
	const double YawX = (1.f - 2.f * ((F->y * F->y) + (F->z * F->z)));

	const double SINGULARITY_THRESHOLD = 0.4999995f;
	const double RAD_TO_DEG = 57.295776;
	float Pitch, Yaw, Roll;

	if (SingularityTest < -SINGULARITY_THRESHOLD)
	{
		Pitch = -90.f;
		Yaw = (Atan2(YawY, YawX) * RAD_TO_DEG);
		Roll = NormalizeAxis(-Yaw - (2.f * Atan2(F->x, F->w) * RAD_TO_DEG));
	}
	else if (SingularityTest > SINGULARITY_THRESHOLD)
	{
		Pitch = 90.f;
		Yaw = (Atan2(YawY, YawX) * RAD_TO_DEG);
		Roll = NormalizeAxis(Yaw - (2.f * Atan2(F->x, F->w) * RAD_TO_DEG));
	}
	else
	{
		Pitch = (asin(2.f * SingularityTest) * RAD_TO_DEG);
		Yaw = (Atan2(YawY, YawX) * RAD_TO_DEG);
		Roll = (Atan2(-2.f * (F->w * F->x + F->y * F->z), (1.f - 2.f * ((F->x * F->x) + (F->y * F->y)))) * RAD_TO_DEG);
	}

	FRotator RotatorFromQuat = FRotator{ Pitch, Yaw, Roll };
	return RotatorFromQuat;
}


struct CamewaDescwipsion
{
	float FieldOfView;
	Vector3 Rotation;
	Vector3 Location;
};
CamewaDescwipsion UndetectedCamera()
{
	CamewaDescwipsion VirtualCamera;
	__int64 ViewStates;
	__int64 ViewState;

	ViewStates = KmDrv->Rpm<__int64>(Localplayer + 0xD0);
	ViewState = KmDrv->Rpm<__int64>(ViewStates + 8);

	VirtualCamera.FieldOfView = KmDrv->Rpm<float>(PlayerController + 0x38C) * 90.f;

	VirtualCamera.Rotation.x = KmDrv->Rpm<double>(ViewState + 0x9C0);
	__int64 ViewportClient = KmDrv->Rpm<__int64>(Localplayer + 0x78);
	if (!ViewportClient) return VirtualCamera;
	__int64 AudioDevice = KmDrv->Rpm<__int64>(ViewportClient + 0x98);
	if (!AudioDevice) return VirtualCamera;
	__int64 FListener = KmDrv->Rpm<__int64>(AudioDevice + 0x1E0);
	if (!FListener) return VirtualCamera;
	FQuat Listener = KmDrv->Rpm<FQuat>(FListener);
	VirtualCamera.Rotation.y = Rotator(&Listener).Yaw;

	VirtualCamera.Location = KmDrv->Rpm<Vector3>(KmDrv->Rpm<uintptr_t>(Uworld + 0x110));
	return VirtualCamera;
}
Vector3 ProjectWorldToScreen(Vector3 WorldLocation)
{
	CamewaDescwipsion vCamera = UndetectedCamera();
	vCamera.Rotation.x = (asin(vCamera.Rotation.x)) * (180.0 / M_PI);

	D3DMATRIX tempMatrix = Matrix(vCamera.Rotation);

	Vector3 vAxisX = Vector3(tempMatrix.m[0][0], tempMatrix.m[0][1], tempMatrix.m[0][2]);
	Vector3 vAxisY = Vector3(tempMatrix.m[1][0], tempMatrix.m[1][1], tempMatrix.m[1][2]);
	Vector3 vAxisZ = Vector3(tempMatrix.m[2][0], tempMatrix.m[2][1], tempMatrix.m[2][2]);

	Vector3 vDelta = WorldLocation - vCamera.Location;
	Vector3 vTransformed = Vector3(vDelta.Dot(vAxisY), vDelta.Dot(vAxisZ), vDelta.Dot(vAxisX));

	if (vTransformed.z < 1.f)
		vTransformed.z = 1.f;

	return Vector3((Width / 2.0f) + vTransformed.x * (((Width / 2.0f) / tanf(vCamera.FieldOfView * (float)M_PI / 360.f))) / vTransformed.z, (Height / 2.0f) - vTransformed.y * (((Width / 2.0f) / tanf(vCamera.FieldOfView * (float)M_PI / 360.f))) / vTransformed.z, 0);
}

void DrawStrokeText(int x, int y, const char* str)
{
	ImFont a;
	std::string utf_8_1 = std::string(str);
	std::string utf_8_2 = string_To_UTF8(utf_8_1);

	ImGui::GetOverlayDrawList()->AddText(ImVec2(x, y - 1), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), utf_8_2.c_str());
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x, y + 1), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), utf_8_2.c_str());
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x - 1, y), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), utf_8_2.c_str());
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x + 1, y), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), utf_8_2.c_str());
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x, y), ImGui::ColorConvertFloat4ToU32(ImVec4(255, 255, 255, 255)), utf_8_2.c_str());
}
Vector3 CalculateNewRotation(Vector3& zaz, Vector3& daz) {
	Vector3 dalte = zaz - daz;
	Vector3 ongle;
	float hpm = sqrtf(dalte.x * dalte.x + dalte.y * dalte.y);
	ongle.y = atan(dalte.y / dalte.x) * 57.295779513082f;
	ongle.x = (atan(dalte.z / hpm) * 57.295779513082f) * -1.f;
	if (dalte.x >= 0.f) ongle.y += 180.f;
	return ongle;
}
void SilentAim(DWORD_PTR closestPawn)
{
	CamewaDescwipsion vCamera = UndetectedCamera();
	uint64_t currentactormesh = KmDrv->Rpm<uint64_t>(closestPawn + OFFSETS::Mesh);
	Vector3 hitbone = GetBoneWithRotation(currentactormesh, 68);
	Vector3 HeadBone = ProjectWorldToScreen(hitbone);
	uintptr_t CurrentWeapon = KmDrv->Rpm<uintptr_t>(LocalPawn + OFFSETS::CurrentWeapon);
	uintptr_t PlayerCameraManager = KmDrv->Rpm<uintptr_t>(PlayerController + 0x340);
	Vector3 NewRotation = CalculateNewRotation(vCamera.Location, HeadBone);

	static float OrginalPitchMin = KmDrv->Rpm<float>(PlayerCameraManager + OFFSETS::cumera::AimPitchMin);
	static float OrginalPitchMax = KmDrv->Rpm<float>(PlayerCameraManager + OFFSETS::cumera::ViewYawMax);

	KmDrv->Wpm<float>(CurrentWeapon + OFFSETS::cumera::AimPitchMin, NewRotation.x);
	KmDrv->Wpm<float>(CurrentWeapon + OFFSETS::cumera::AimPitchMax, NewRotation.x);

	KmDrv->Wpm<float>(PlayerCameraManager + OFFSETS::cumera::ViewYawMin, NewRotation.y);
	KmDrv->Wpm<float>(PlayerCameraManager + OFFSETS::cumera::ViewYawMax, NewRotation.y);

	std::this_thread::sleep_for(std::chrono::milliseconds(5));

	KmDrv->Wpm<float>(PlayerCameraManager + OFFSETS::cumera::ViewYawMin, OrginalPitchMin);
	KmDrv->Wpm<float>(PlayerCameraManager + OFFSETS::cumera::ViewYawMax, OrginalPitchMax);
}

float DrawOutlinedText(ImFont* pFont, const std::string& text, const ImVec2& pos, float size, ImU32 color, bool center)
{
	std::stringstream stream(text);
	std::string line;

	float y = 0.0f;
	int i = 0;

	while (std::getline(stream, line))
	{
		ImVec2 textSize = pFont->CalcTextSizeA(size, FLT_MAX, 0.0f, line.c_str());

		if (center)
		{
			ImGui::GetOverlayDrawList()->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f) + 1, (pos.y + textSize.y * i) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			ImGui::GetOverlayDrawList()->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f) - 1, (pos.y + textSize.y * i) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			ImGui::GetOverlayDrawList()->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f) + 1, (pos.y + textSize.y * i) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			ImGui::GetOverlayDrawList()->AddText(pFont, size, ImVec2((pos.x - textSize.x / 2.0f) - 1, (pos.y + textSize.y * i) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());

			ImGui::GetOverlayDrawList()->AddText(pFont, size, ImVec2(pos.x - textSize.x / 2.0f, pos.y + textSize.y * i), ImGui::GetColorU32(color), line.c_str());
		}
		else
		{
			ImGui::GetOverlayDrawList()->AddText(pFont, size, ImVec2((pos.x) + 1, (pos.y + textSize.y * i) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			ImGui::GetOverlayDrawList()->AddText(pFont, size, ImVec2((pos.x) - 1, (pos.y + textSize.y * i) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			ImGui::GetOverlayDrawList()->AddText(pFont, size, ImVec2((pos.x) + 1, (pos.y + textSize.y * i) - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());
			ImGui::GetOverlayDrawList()->AddText(pFont, size, ImVec2((pos.x) - 1, (pos.y + textSize.y * i) + 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 255)), line.c_str());

			ImGui::GetOverlayDrawList()->AddText(pFont, size, ImVec2(pos.x, pos.y + textSize.y * i), ImGui::GetColorU32(color), line.c_str());
		}

		y = pos.y + textSize.y * (i + 1);
		i++;
	}
	return y;
}

void DrawText1(int x, int y, const char* str, RGBA* color)
{
	ImFont a;
	std::string utf_8_1 = std::string(str);
	std::string utf_8_2 = string_To_UTF8(utf_8_1);
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x, y), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), utf_8_2.c_str());
}
auto KeyEvent(bool switch1)
{
	switch (switch1)
	{
	case true:
		return keybd_event(VK_LBUTTON, 0x45, KEYEVENTF_EXTENDEDKEY, 0);
		break;
	case false:
		return keybd_event(VK_LBUTTON, 0x45, KEYEVENTF_KEYUP, 0);
		break;
	}
}
void Trigger_Event(uintptr_t CurrentActorMesh)
{
	Vector3 hitbone = GetBoneWithRotation(CurrentActorMesh, hitboxpos);
	Vector3 hitbone1 = GetBoneWithRotation(CurrentActorMesh, 0);
	Vector3 RootBone = ProjectWorldToScreen(hitbone1);
	Vector3 HeadBone = ProjectWorldToScreen(hitbone);

	if (GetAsyncKeyState(VK_LBUTTON)) {
		if (HeadBone.x != 0 || HeadBone.y != 0 || HeadBone.z != 0)
			KeyEvent(true);
		else if (RootBone.y or RootBone.z)
			KeyEvent(true);
		else
			KeyEvent(false);
	}
}

void DrawLine(int x1, int y1, int x2, int y2, RGBA* color, int thickness)
{
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), thickness);
}
void DrawCircle(int x, int y, int radius, RGBA* color, int segments)
{
	ImGui::GetOverlayDrawList()->AddCircle(ImVec2(x, y), radius, ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), segments);
}
//pasta made by Zyko#5528
void DrawBox(float X, float Y, float W, float H, RGBA* color)
{
	ImGui::GetOverlayDrawList()->AddRect(ImVec2(X + 1, Y + 1), ImVec2(((X + W) - 1), ((Y + H) - 1)), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)));
	ImGui::GetOverlayDrawList()->AddRect(ImVec2(X, Y), ImVec2(X + W, Y + H), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)));
}
void DrawFilledRect(int x, int y, int w, int h, ImU32 color)
{
	ImGui::GetOverlayDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), color, 0, 0);
}
void DrawNormalBox(int x, int y, int w, int h, int borderPx, ImU32 color)
{

	DrawFilledRect(x + borderPx, y, w, borderPx, color); //top 
	DrawFilledRect(x + w - w + borderPx, y, w, borderPx, color); //top 
	DrawFilledRect(x, y, borderPx, h, color); //left 
	DrawFilledRect(x, y + h - h + borderPx * 2, borderPx, h, color); //left 
	DrawFilledRect(x + borderPx, y + h + borderPx, w, borderPx, color); //bottom 
	DrawFilledRect(x + w - w + borderPx, y + h + borderPx, w, borderPx, color); //bottom 
	DrawFilledRect(x + w + borderPx, y, borderPx, h, color);//right 
	DrawFilledRect(x + w + borderPx, y + h - h + borderPx * 2, borderPx, h, color);//right 
}
void DrawCorneredBox(int X, int Y, int W, int H, const ImU32& color, int thickness) {
	float lineW = (W / 3);
	float lineH = (H / 3);

	//black outlines
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X, Y), ImVec2(X, Y + lineH), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X, Y), ImVec2(X + lineW, Y), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X + W - lineW, Y), ImVec2(X + W, Y), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X + W, Y), ImVec2(X + W, Y + lineH), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X, Y + H - lineH), ImVec2(X, Y + H), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X, Y + H), ImVec2(X + lineW, Y + H), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X + W - lineW, Y + H), ImVec2(X + W, Y + H), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X + W, Y + H - lineH), ImVec2(X + W, Y + H), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), 3);

	//corners
	//pasta made by Zyko#5528
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X, Y), ImVec2(X, Y + lineH), ImGui::GetColorU32(color), thickness);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X, Y), ImVec2(X + lineW, Y), ImGui::GetColorU32(color), thickness);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X + W - lineW, Y), ImVec2(X + W, Y), ImGui::GetColorU32(color), thickness);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X + W, Y), ImVec2(X + W, Y + lineH), ImGui::GetColorU32(color), thickness);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X, Y + H - lineH), ImVec2(X, Y + H), ImGui::GetColorU32(color), thickness);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X, Y + H), ImVec2(X + lineW, Y + H), ImGui::GetColorU32(color), thickness);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X + W - lineW, Y + H), ImVec2(X + W, Y + H), ImGui::GetColorU32(color), thickness);
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(X + W, Y + H - lineH), ImVec2(X + W, Y + H), ImGui::GetColorU32(color), thickness);
}




typedef struct _FNlEntity
{
	uint64_t Actor;
	int ID;
	uint64_t mesh;
}FNlEntity;

std::vector<FNlEntity> entityList;


struct HandleDisposer
{
	using pointer = HANDLE;
	void operator()(HANDLE handle) const
	{
		if (handle != NULL || handle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(handle);
		}
	}
};
using unique_handle = std::unique_ptr<HANDLE, HandleDisposer>;

static std::uint32_t _GetProcessId(std::string process_name) {
	PROCESSENTRY32 processentry;
	const unique_handle snapshot_handle(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));

	if (snapshot_handle.get() == INVALID_HANDLE_VALUE)
		return 0;

	processentry.dwSize = sizeof(MODULEENTRY32);

	while (Process32Next(snapshot_handle.get(), &processentry) == TRUE) {
		if (process_name.compare(processentry.szExeFile) == 0)
			return processentry.th32ProcessID;
	}
	return 0;
}

#include <sstream>

template <typename T>
void readValue(const nlohmann::json& src, T& dest)
{
	if (!src.is_null())
	{
		if (src.is_string())
		{
			std::string value_str = src.get<std::string>();
			if (value_str.substr(0, 2) == "0x") // hex prefix
			{
				dest = static_cast<T>(std::stoul(value_str.substr(2), nullptr, 16));
			}
			else
			{
				dest = static_cast<T>(std::stoul(value_str, nullptr, 10));
			}
		}
		else
		{
			dest = src.get<T>();
		}
	}
}


void autoupdate()
{
	std::ifstream input_file{ "C:\\Windows\\offsets.json" };
	if (!input_file.good())
		throw std::invalid_argument("Invalid offsets.json file");

	nlohmann::json json;
	input_file >> json;

	readValue(json["offsets"]["uworld"], OFFSET_UWORLD);
	readValue(json["offsets"]["Gameinstance"], OFFSETS::Gameinstance);
	readValue(json["offsets"]["LocalPlayers"], OFFSETS::LocalPlayers);
	readValue(json["offsets"]["PlayerController"], OFFSETS::PlayerController);
	readValue(json["offsets"]["LocalPawn"], OFFSETS::LocalPawn);
	readValue(json["offsets"]["PlayerState"], OFFSETS::PlayerState);
	readValue(json["offsets"]["RootComponet"], OFFSETS::RootComponet);
	readValue(json["offsets"]["GameState"], OFFSETS::GameState);
	readValue(json["offsets"]["PersistentLevel"], OFFSETS::PersistentLevel);
	readValue(json["offsets"]["ActorCount"], OFFSETS::ActorCount);
	readValue(json["offsets"]["Cameramanager"], OFFSETS::Cameramanager);
	readValue(json["offsets"]["AActor"], OFFSETS::AActor);
	readValue(json["offsets"]["CurrentActor"], OFFSETS::CurrentActor);
	readValue(json["offsets"]["Mesh"], OFFSETS::Mesh);
	readValue(json["offsets"]["Revivefromdbnotime"], OFFSETS::Revivefromdbnotime);
	readValue(json["offsets"]["TeamId"], OFFSETS::TeamId);
	readValue(json["offsets"]["ActorTeamId"], OFFSETS::ActorTeamId);
	readValue(json["offsets"]["IsDBNO"], OFFSETS::IsDBNO);
	readValue(json["offsets"]["LocalActorPos"], OFFSETS::LocalActorPos);
	readValue(json["offsets"]["ComponetToWorld"], OFFSETS::ComponetToWorld);
	readValue(json["offsets"]["BoneArray"], OFFSETS::BoneArray);
	readValue(json["offsets"]["Velocity"], OFFSETS::Velocity);
	readValue(json["offsets"]["PawnPrivate"], OFFSETS::PawnPrivate);
	readValue(json["offsets"]["PlayerArray"], OFFSETS::PlayerArray);
	readValue(json["offsets"]["relativelocation"], OFFSETS::relativelocation);
	readValue(json["offsets"]["UCharacterMovementComponent"], OFFSETS::UCharacterMovementComponent);
	readValue(json["offsets"]["entity_actor"], OFFSETS::entity_actor);
	readValue(json["offsets"]["bIsReloadingWeapon"], OFFSETS::bIsReloadingWeapon);
	readValue(json["offsets"]["GlobalAnimRateScale"], OFFSETS::GlobalAnimRateScale);
	readValue(json["offsets"]["CurrentWeapon"], OFFSETS::CurrentWeapon);
}
int main(int argc, const char* argv[])
{
	const char* url = (argc > 1 ? argv[1] : ("https://raw.githubusercontent.com/exploitfn/fortnite-always-updated-offsets/main/offsets.json"));
	HRESULT result = URLDownloadToFileA(nullptr, url, ("C:\\Windows\\offsets.json"), 0, nullptr);
	if (result != S_OK)
	{
		std::cout << ("[ ud man ] Could not update offsets. Error code: ") << std::hex << result << std::endl;
	}
	else
	{
		std::cout << ("[ ud man ] Offsets successfully Updated.") << std::endl;
	}

	autoupdate(); // your cheat will only work whenever i update these offsets


	MouseController::Init();

	CreateThread(NULL, NULL, Menuthread, NULL, NULL, NULL);
	//AttachDriver();
	std::cout << "\n\n  [+] Open Fortnite!";
	while (hwnd == NULL)
	{
		XorS(wind, "Fortnite  ");
		hwnd = FindWindowA(0, wind.decrypt());
		Sleep(100);
	}

	//processID = _GetProcessId("FortniteClient-Win64-Shipping.exe");
	GetWindowThreadProcessId(hwnd, &processID);
	KmDrv = new Memory(processID);
	base_address = KmDrv->GetModuleBase("FortniteClient-Win64-Shipping.exe");
	std::cout << ("\n\n  [+] Module Base: ") << std::hex << base_address;
	xCreateWindow();
	xInitD3d();

	xMainLoop();
	xShutdown();

	return 0;
}

void SetWindowToTarget()
{
	while (true)
	{
		if (hwnd)
		{
			ZeroMemory(&GameRect, sizeof(GameRect));
			GetWindowRect(hwnd, &GameRect);
			Width = GameRect.right - GameRect.left;
			Height = GameRect.bottom - GameRect.top;
			DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);

			if (dwStyle & WS_BORDER)
			{
				GameRect.top += 32;
				Height -= 39;
			}
			ScreenCenterX = Width / 2;
			ScreenCenterY = Height / 2;
			MoveWindow(Window, GameRect.left, GameRect.top, Width, Height, true);
		}
		else
		{
			exit(0);
		}
	}
}

const MARGINS Margin = { -1 };

void xCreateWindow()
{
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)SetWindowToTarget, 0, 0, 0);

	WNDCLASS windowClass = { 0 };
	windowClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.hInstance = NULL;
	windowClass.lpfnWndProc = WinProc;
	windowClass.lpszClassName = "Zyko#5528";
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	if (!RegisterClass(&windowClass))
		std::cout << "\n\n [-] github src by Zyko#5528 overlay failed! :(";

	Window = CreateWindow("Zyko#5528",
		NULL,
		WS_POPUP,
		0,
		0,
		GetSystemMetrics(SM_CXSCREEN),
		GetSystemMetrics(SM_CYSCREEN),
		NULL,
		NULL,
		NULL,
		NULL);

	ShowWindow(Window, SW_SHOW);

	DwmExtendFrameIntoClientArea(Window, &Margin);
	SetWindowLong(Window, GWL_EXSTYLE, WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_LAYERED);
	UpdateWindow(Window);
}

void xInitD3d()
{
	if (FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION, &p_Object)))
		exit(3);

	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.BackBufferWidth = Width;
	d3dpp.BackBufferHeight = Height;
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	d3dpp.MultiSampleQuality = D3DMULTISAMPLE_NONE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.hDeviceWindow = Window;
	d3dpp.Windowed = TRUE;

	p_Object->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, Window, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &D3dDevice);

	IMGUI_CHECKVERSION();

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	io.IniFilename = NULL;

	ImGui_ImplWin32_Init(Window);
	ImGui_ImplDX9_Init(D3dDevice);

	ImGui::StyleColorsClassic();
	ImGuiStyle* style = &ImGui::GetStyle();

	style->WindowPadding = ImVec2(15, 15);
	style->WindowRounding = 0.0f;
	style->FramePadding = ImVec2(5, 5);
	style->FrameRounding = 0.0f;
	style->ItemSpacing = ImVec2(12, 8);
	style->ItemInnerSpacing = ImVec2(8, 6);
	style->IndentSpacing = 25.0f;
	style->ScrollbarSize = 15.0f;
	style->ScrollbarRounding = 0.0f;
	style->GrabMinSize = 5.0f;
	style->GrabRounding = 0.0f;

	style->Colors[ImGuiCol_Text] = ImColor(255, 255 ,255, 255);
	style->Colors[ImGuiCol_TextDisabled] = ImColor(255, 255, 255, 255);
	style->Colors[ImGuiCol_WindowBg] = ImColor(30, 30, 30, 255);
	style->Colors[ImGuiCol_ChildWindowBg] = ImColor(255, 255, 255, 255);
	style->Colors[ImGuiCol_PopupBg] = ImColor(50, 50, 50, 255);
	style->Colors[ImGuiCol_Border] = ImColor(255, 0, 255, 255);
	style->Colors[ImGuiCol_BorderShadow] = ImColor(0, 0, 0, 255);
	style->Colors[ImGuiCol_FrameBg] = ImColor(50, 50, 50, 255);
	style->Colors[ImGuiCol_FrameBgHovered] = ImColor(70, 70, 70, 255);
	style->Colors[ImGuiCol_FrameBgActive] = ImColor(90, 90, 90, 255);
	style->Colors[ImGuiCol_TitleBg] = ImColor(30, 30, 30, 255);
	style->Colors[ImGuiCol_TitleBgCollapsed] = ImColor(30, 30, 30, 255);
	style->Colors[ImGuiCol_TitleBgActive] = ImColor(30, 30, 30, 255);
	style->Colors[ImGuiCol_MenuBarBg] = ImColor(255, 255, 255, 255);
	style->Colors[ImGuiCol_ScrollbarBg] = ImColor(100, 100, 100, 255);
	style->Colors[ImGuiCol_ScrollbarGrab] = ImColor(60, 60, 60, 255);
	style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImColor(70, 70, 70, 255);
	style->Colors[ImGuiCol_ScrollbarGrabActive] = ImColor(80, 80, 80, 255);
	style->Colors[ImGuiCol_CheckMark] = ImColor(0, 255, 255, 255);
	style->Colors[ImGuiCol_SliderGrab] = ImColor(255, 255, 255, 255);
	style->Colors[ImGuiCol_SliderGrabActive] = ImColor(255, 255, 255, 255);
	style->Colors[ImGuiCol_Button] = ImColor(50, 50, 50, 255);
	style->Colors[ImGuiCol_ButtonHovered] = ImColor(70, 70, 70, 255);
	style->Colors[ImGuiCol_ButtonActive] = ImColor(90, 90, 90, 255);
	style->Colors[ImGuiCol_Header] = ImColor(50, 50, 50, 255);
	style->Colors[ImGuiCol_HeaderHovered] = ImColor(50, 50, 50, 255);
	style->Colors[ImGuiCol_HeaderActive] = ImColor(50, 50, 50, 255);
	style->Colors[ImGuiCol_Column] = ImColor(255, 255, 255, 255);
	style->Colors[ImGuiCol_ColumnHovered] = ImColor(255, 255, 255, 255);
	style->Colors[ImGuiCol_ColumnActive] = ImColor(255, 255, 255, 255);
	style->Colors[ImGuiCol_ResizeGrip] = ImColor(255, 255, 255, 255);
	style->Colors[ImGuiCol_ResizeGripHovered] = ImColor(255, 255, 255, 255);
	style->Colors[ImGuiCol_ResizeGripActive] = ImColor(255, 255, 255, 255);
	style->Colors[ImGuiCol_PlotLines] = ImColor(255, 255, 255, 255);
	style->Colors[ImGuiCol_PlotLinesHovered] = ImColor(255, 255, 255, 255);
	style->Colors[ImGuiCol_PlotHistogram] = ImColor(255, 255, 255, 255);
	style->Colors[ImGuiCol_PlotHistogramHovered] = ImColor(255, 255, 255, 255);
	style->Colors[ImGuiCol_TextSelectedBg] = ImColor(255, 255, 255, 255);
	style->Colors[ImGuiCol_ModalWindowDarkening] = ImColor(255, 255, 255, 255);

	style->WindowTitleAlign.x = 0.5f;
	style->FrameRounding = 0.0f;

	XorS(font, "C:\\Windows\\Fonts\\Arial.ttf");
	m_pFont = io.Fonts->AddFontFromFileTTF(font.decrypt(), 14.0f, nullptr, io.Fonts->GetGlyphRangesDefault());

	p_Object->Release();
}
double NRX;
double NRY;
void memory(float x, float y, DWORD_PTR playercontroller)
{
	int AimSpeed = memorysmoothing;

	float screen_center_x = (ImGui::GetIO().DisplaySize.x / 2);
	float screen_center_y = (ImGui::GetIO().DisplaySize.y / 2);
	float TargetX = 0;
	float TargetY = 0;
	if (x != 0)
	{
		if
			(x > screen_center_x)
		{
			TargetX = -(screen_center_x - x); if (TargetX + screen_center_x > screen_center_x * 2)TargetX = 0;
		}
		if
			(x < screen_center_x)
		{
			TargetX = x - screen_center_x; if (TargetX + screen_center_x < 0)TargetX = 0;
		}
	}
	if (y != 0)
	{
		if
			(y > screen_center_y)
		{
			TargetY = -(screen_center_y - y); if (TargetY + screen_center_y > screen_center_y * 2) TargetY = 0;
		}
		if
			(y < screen_center_y)
		{
			TargetY = y - screen_center_y; if (TargetY + screen_center_y < 0) TargetY = 0;
		}
	}

	Vector3 Angles;
	Angles = Vector3(-TargetY / AimSpeed, TargetX / AimSpeed, 0);

	KmDrv->Wpm<double>(PlayerController + 0x518, Angles.x);
	KmDrv->Wpm<double>(PlayerController + 0x518 + 0x8, Angles.y);

}
void aimbot(float x, float y)
{
	float ScreenCenterX = (Width / 2);
	float ScreenCenterY = (Height / 2);
	int AimSpeed = smooth;
	float TargetX = 0;
	float TargetY = 0;

	if (x != 0)
	{
		if (x > ScreenCenterX)
		{
			TargetX = -(ScreenCenterX - x);
			TargetX /= AimSpeed;
			if (TargetX + ScreenCenterX > ScreenCenterX * 2) TargetX = 0;
		}

		if (x < ScreenCenterX)
		{
			TargetX = x - ScreenCenterX;
			TargetX /= AimSpeed;
			if (TargetX + ScreenCenterX < 0) TargetX = 0;
		}
	}

	if (y != 0)
	{
		if (y > ScreenCenterY)
		{
			TargetY = -(ScreenCenterY - y);
			TargetY /= AimSpeed;
			if (TargetY + ScreenCenterY > ScreenCenterY * 2) TargetY = 0;
		}

		if (y < ScreenCenterY)
		{
			TargetY = y - ScreenCenterY;
			TargetY /= AimSpeed;
			if (TargetY + ScreenCenterY < 0) TargetY = 0;
		}
	}

	mouse_event(MOUSEEVENTF_MOVE, static_cast<DWORD>(TargetX), static_cast<DWORD>(TargetY), NULL, NULL);
	return;
}
void AimAt(DWORD_PTR entity)
{
	uint64_t currentactormesh = KmDrv->Rpm<uint64_t>(entity + 0x2F0);
	auto rootHead = GetBoneWithRotation(currentactormesh, hitbox);
	Vector3 rootHeadOut = ProjectWorldToScreen(rootHead);
	//Pasta made by Zyko#5528
	if (rootHeadOut.y != 0 || rootHeadOut.y != 0)
	{
		aimbot(rootHeadOut.x, rootHeadOut.y);
	}
}

bool isVisible(DWORD_PTR mesh)
{
	if (!mesh)
		return false;
	float fLastSubmitTime = KmDrv->Rpm<float>(mesh + 0x360);// + 20
	float fLastRenderTimeOnScreen = KmDrv->Rpm<float>(mesh + 0x368);// + 24
	const float fVisionTick = 0.06f;
	bool bVisible = fLastRenderTimeOnScreen + fVisionTick >= fLastSubmitTime;
	return bVisible;
}




void reset_angles() {
	float ViewPitchMin = -89.9999f;
	float ViewPitchMax = 89.9999f;
	float ViewYawMin = 0.0000f;
	float ViewYawMax = 359.9999f;

	uintptr_t PlayerCameraManager = KmDrv->Rpm<uintptr_t>(PlayerController + 0x340);
	KmDrv->Wpm<float>(PlayerCameraManager + OFFSETS::cumera::AimPitchMin, ViewPitchMin);
	KmDrv->Wpm<float>(PlayerCameraManager + OFFSETS::cumera::ViewYawMax, ViewPitchMax);
	KmDrv->Wpm<float>(PlayerCameraManager + OFFSETS::cumera::AimPitchMin, ViewYawMin);
	KmDrv->Wpm<float>(PlayerCameraManager + OFFSETS::cumera::ViewYawMax, ViewYawMax);
}
static auto DrawCircleFilled(int x, int y, int radius, RGBA* color) -> void
{
	ImGui::GetOverlayDrawList()->AddCircleFilled(ImVec2(x, y), radius, ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)));
}
namespace cumera
{
	Vector3 Location;
};
Vector3 GetLoc(Vector3 Loc) {
	return Vector3(Loc.x, Loc.y, Loc.z);
}
Vector3 AimbotCorrection(float bulletVelocity, float bulletGravity, float targetDistance, Vector3 targetPosition, Vector3 targetVelocity) {
	Vector3 recalculated = targetPosition;
	float gravity = fabs(bulletGravity);
	float time = targetDistance / fabs(bulletVelocity);
	float bulletDrop = (gravity / 250) * time * time;
	recalculated.z += bulletDrop * 120;
	recalculated.x += time * (targetVelocity.x);
	recalculated.y += time * (targetVelocity.y);
	recalculated.z += time * (targetVelocity.z);
	return recalculated;
}

void Aim_Prediction(DWORD_PTR Closestpawn)
{
	uintptr_t CurrentActorMesh = KmDrv->Rpm<uintptr_t>(Closestpawn + OFFSETS::Mesh);
	Vector3 hitbone = GetBoneWithRotation(CurrentActorMesh, 68);
	Vector3 HeadBone = ProjectWorldToScreen(GetLoc(hitbone));
	float distance = localactorpos.Distance(HeadBone) / AimFOV;
	uint64_t CurrentActorRootComponent = KmDrv->Rpm<uint64_t>(OFFSETS::CurrentActor + 0x190);
	Vector3 vellocity = KmDrv->Rpm<Vector3>(CurrentActorRootComponent + OFFSETS::Velocity);
	Vector3 Predicted = AimbotCorrection(-504, 35, distance, HeadBone, vellocity);
	Vector3 rootHeadOut = ProjectWorldToScreen(Predicted);
	aimbot(rootHeadOut.x, rootHeadOut.y);
}
double GetCrossDistance(double x1, double y1, double x2, double y2) {
	return sqrt(pow((x2 - x1), 2) + pow((y2 - y1), 2));
}
void OutlinedText(int x, int y, ImColor Color, const char* text)
{
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x - 1, y), ImColor(0, 0, 0), text);
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x + 1, y), ImColor(0, 0, 0), text);
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x, y - 1), ImColor(0, 0, 0), text);
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x, y + 1), ImColor(0, 0, 0), text);
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x, y), Color, text);
}
void HeadCircle(int x, int y, int radius, RGBA* color, int segments)
{
	ImGui::GetOverlayDrawList()->AddCircle(ImVec2(x, y), radius, ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), segments);
}
void FovCircle(int x, int y, int radius, RGBA* color, int segments, float thickness)
{
	ImGui::GetOverlayDrawList()->AddCircle(ImVec2(x, y), radius, ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), segments, thickness);
}
void drawfilledbox(int x, int y, int w, int h, RGBA* color)
{
	ImGui::GetOverlayDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), 0, 0);
}
Vector3 Headbox;
float distance = 100.f;
static float headcolor[3] = { 1.0f , 0.0f , 0.0f };
RGBA head = { headcolor[0] * 255, headcolor[1] * 255, headcolor[2] * 255, 255 };
uint64_t CurrentActorMesh;
void DrawESP() {
	if (fovcircle) {
		FovCircle(ScreenCenterX, ScreenCenterY, AimFOV, &Col.purple, 150, 1);
		
	}
	if (hitboxpos == 0)
	{
		hitbox = 68; //head
	}
	else if (hitboxpos == 1)
	{
		hitbox = 66; //neck
	}
	else if (hitboxpos == 2)
	{
		hitbox = 5; //chest
	}
	else if (hitboxpos == 3)
	{
		hitbox = 2; //pelvis
	}
	//auto entityListCopy = entityList;
	float closestDistance = FLT_MAX;
	DWORD_PTR closestPawn = NULL;
	Uworld = KmDrv->Rpm<DWORD_PTR>(base_address + OFFSET_UWORLD);
	DWORD_PTR Gameinstance = KmDrv->Rpm<DWORD_PTR>(Uworld + OFFSETS::Gameinstance);
	DWORD_PTR LocalPlayers = KmDrv->Rpm<DWORD_PTR>(Gameinstance + OFFSETS::LocalPlayers);
	Localplayer = KmDrv->Rpm<DWORD_PTR>(LocalPlayers);
	PlayerController = KmDrv->Rpm<DWORD_PTR>(Localplayer + OFFSETS::PlayerController);
	LocalPawn = KmDrv->Rpm<DWORD_PTR>(PlayerController + OFFSETS::LocalPawn);
	//PlayerCameraManager = KmDrv->Rpm<uintptr_t >(PlayerController + 0x340); // 0x340
	PlayerState = KmDrv->Rpm<DWORD_PTR>(LocalPawn + OFFSETS::PlayerState);
	Rootcomp = KmDrv->Rpm<DWORD_PTR>(LocalPawn + OFFSETS::RootComponet);
	Persistentlevel = KmDrv->Rpm<DWORD_PTR>(Uworld + OFFSETS::PersistentLevel);
	DWORD ActorCount = KmDrv->Rpm<DWORD>(Persistentlevel + OFFSETS::ActorCount);
	DWORD_PTR AActors = KmDrv->Rpm<DWORD_PTR>(Persistentlevel + OFFSETS::AActor);

	if (streamproof)
	{
		SetWindowDisplayAffinity(Window, WDA_EXCLUDEFROMCAPTURE);
	}
	else {
		SetWindowDisplayAffinity(Window, !WDA_EXCLUDEFROMCAPTURE);
	}
	for (int i = 0; i < ActorCount; i++)
	{
		uint64_t CurrentActor = KmDrv->Rpm<uint64_t>(AActors + i * 0x8);
		
        if(KmDrv->Rpm<float>(CurrentActor + OFFSETS::Revivefromdbnotime) != 10) continue;
		CurrentActorMesh = KmDrv->Rpm<uint64_t>(CurrentActor + OFFSETS::Mesh);
		int MyTeamId = KmDrv->Rpm<int>(PlayerState + OFFSETS::TeamId);
		DWORD64 otherPlayerState = KmDrv->Rpm<uint64_t>(CurrentActor + OFFSETS::PlayerState);
		int ActorTeamId = KmDrv->Rpm<int>(otherPlayerState + OFFSETS::ActorTeamId);
		//auto isDBNO = (KmDrv->Rpm<char>(CurrentActor + 0x74a) >> 4) & 1;
        if (MyTeamId == ActorTeamId) continue;
		if (CurrentActor == LocalPawn) continue;
			//localactorpos = KmDrv->Rpm<Vector3>(Rootcomp + 0x128);
			Vector3 Headpos = GetBoneWithRotation(CurrentActorMesh, 68);
			Vector3 bone0 = GetBoneWithRotation(CurrentActorMesh, 0);
			Vector3 bottom = ProjectWorldToScreen(bone0);
			Headbox = ProjectWorldToScreen(Vector3(Headpos.x, Headpos.y, Headpos.z + 15));
			Vector3 w2shead = ProjectWorldToScreen(Headpos);
			//auto player_screen = ProjectWorldToScreen(bone0);
			float BoxHeight = (float)(Headbox.y - bottom.y);
			//float BoxWidth = BoxHeight * 0.380f;
			//float LeftX = (float)Headbox.x - (BoxWidth / 1);
			//float LeftY = (float)bottom.y;
			float CornerHeight = abs(Headbox.y - bottom.y);
			float CornerWidth = CornerHeight * 0.50;
			if (distance < VisDist)
			{
				if (Esp)
				{
					if (Esp_box)
					{
						if (isVisible(CurrentActorMesh))
						{
							DrawNormalBox(bottom.x - (CornerWidth / 2), Headbox.y, CornerWidth, CornerHeight, 1, IM_COL32(255, 0, 0, 255));

						}
						else if (!isVisible(CurrentActorMesh))
						{
							DrawNormalBox(bottom.x - (CornerWidth / 2), Headbox.y, CornerWidth, CornerHeight, 1, IM_COL32(255, 255, 255, 255));
						}
					}
					if (cornerbox)
					{
						RGBA filledbox = { 0, 0, 0, 125 };
						DrawCorneredBox(Headbox.x - (CornerWidth / 2), Headbox.y, CornerWidth, CornerHeight, IM_COL32(0, 255, 231, 0), 2.5);
						drawfilledbox(Headbox.x - (CornerWidth / 2), Headbox.y, CornerWidth, CornerHeight, &Col.filledbox);
						if (isVisible(CurrentActorMesh))
						{
							DrawCorneredBox(Headbox.x - (CornerWidth / 2), Headbox.y, CornerWidth, CornerHeight, IM_COL32(255, 0, 0, 255), 1.5);
						}
						else if (!isVisible(CurrentActorMesh))
						{
							DrawCorneredBox(Headbox.x - (CornerWidth / 2), Headbox.y, CornerWidth, CornerHeight, IM_COL32(255, 255, 255, 255), 1.5);
						}
					}
					
					if (Esp_line)
					{
						if (isVisible(CurrentActorMesh)) 
						{
							DrawLine(Width / 2, Height / 1, bottom.x, bottom.y, &Col.red, 1.5);
						}
						if (!isVisible(CurrentActorMesh))
						{
							DrawLine(Width / 2, Height / 1, bottom.x, bottom.y, &Col.white, 1.5);
						}
					}

					if (headdot)
					{
						HeadCircle(Headbox.x - 10, Headbox.y + 30, (float)(Headbox.y - bottom.y) / 30, &head, 100);
					}
				}
			}
			auto dx = w2shead.x - (Width / 2);
			auto dy = w2shead.y - (Height / 2);
			auto dist = sqrtf(dx * dx + dy * dy);
		
			
			if (isVisible(CurrentActorMesh))
			{
				if (dist < AimFOV && dist < closestDistance) {
					closestDistance = dist;
					closestPawn = CurrentActor;
				}
			}
	}

	if (Esp)
	{
		if (aimline && closestPawn)
		{
			if (Headbox.y != 0 || Headbox.y != 0)
			{
				ImGui::GetOverlayDrawList()->AddLine(ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y / 2), ImVec2(Headbox.x, Headbox.y), ImGui::GetColorU32({ 255, 255, 255, 1.0f }), 0.5);
			}
		}
	}

	if (memoryaimbot)
	{
		if (closestPawn && GetAsyncKeyState(VK_RBUTTON) < 0) {
			if (Headbox.y != 0 || Headbox.y != 0)
			{
				memory(Headbox.x, Headbox.y, PlayerController);

			}
		}
	}

	bool ResetAngles;
	if (ResetAngles = true) {
		reset_angles();
		ResetAngles = { false };
	}

	if (silent)
	{
		if (silent && GetAsyncKeyState(VK_LBUTTON) && closestPawn && LocalPawn) {
			SilentAim(closestPawn);
			ResetAngles = { true };
		}
	}

	if (prediction)
	{
		if (prediction && GetAsyncKeyState(VK_RBUTTON) && closestPawn && LocalPawn) {
			Aim_Prediction(closestPawn);
		}
	}
	
	if (Aimbot)
	{
		if (Aimbot && closestPawn && GetAsyncKeyState(VK_RBUTTON) < 0) {
			AimAt(closestPawn);
		}
	}

	if (triggerbot)
	{
		if (triggerbot && closestPawn && LocalPawn) {
			Trigger_Event(CurrentActorMesh);
			PressingTrigger = { true };
		}
	}

	//Sleep(1);
}
bool lol;
bool lol1;
void render() {
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	static int maintabs = 0;
	
	if (ShowMenu)
	{
	
		XorS(menu, "Reverse.cc Updated By Ud Man");
		ImGui::Begin(menu.decrypt(), &ShowMenu, ImVec2(395, 480), 1.0f, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		GradientLine::Render(ImVec2(ImGui::GetContentRegionAvail().x * 2, 3));
		ImGui::SetCursorPos({ 5.f,40.f });
		if (ImGui::Button("Aimbot", ImVec2(120, 28)))
		{
			maintabs = 0;
		}
		ImGui::SetCursorPos({ 147.f,40.f });
		if (ImGui::Button("ESP", ImVec2(120, 28)))
		{
			maintabs = 1;
		}
		ImGui::SetCursorPos({ 289.f,40.f });
		if (ImGui::Button("Other", ImVec2(120, 28)))
		{
			maintabs = 2;
		}
		if (maintabs == 0) {
			XorS(aim1, "Aimbot");
			XorS(aim2, "Memory Movement");
			XorS(aim5, "Fov Circle");
			XorS(smoothh, "Smoothing");
			XorS(smoothhh, "Memory Smoothing");
			XorS(silentem, "Silent");
			XorS(predict, "Prediction");
			XorS(triga, "Triggerbot");
			
			XorS(aim6, "Aim Fov");
			if (ImGui::Toggle(aim1.decrypt(), &Aimbot))
			{
				lol = true;
			}
			if (ImGui::Toggle(aim2.decrypt(), &memoryaimbot))
			{
				lol1 = true;
			}
			if (lol)
			{
				memoryaimbot = false;
				Aimbot = true;
				lol = false;
			}
			if (lol1)
			{
				Aimbot = false;
				memoryaimbot = true;
				lol1 = false;
			}

			ImGui::Toggle(silentem.decrypt(), &silent);
			ImGui::Toggle(predict.decrypt(), &prediction);
			ImGui::Toggle(triga.decrypt(), &triggerbot);

			ImGui::Toggle(aim5.decrypt(), &fovcircle);
			ImGui::Text(aim6.decrypt());
			ImGui::SliderFloat("##thgdrhjt", &AimFOV, 25, 500);
			ImGui::Text(smoothh.decrypt());
			ImGui::SliderFloat("##rgsehydt", &smooth, 3, 15);
			ImGui::Text(smoothhh.decrypt());
			ImGui::SliderFloat("##agdage", &memorysmoothing, 15, 50);
			ImGui::Combo("##tgrsajhggd", &hitboxpos, hitboxes, sizeof(hitboxes) / sizeof(*hitboxes));
		}
		if (maintabs == 1) {
			XorS(box_esp, "2D Box");
			XorS(corner_esp, "2D Corner Box");
			XorS(snapline, "Snaplines");
			XorS(es5, "Max Visuals Distance");
			XorS(aemline, "Aim Line");
			XorS(heddot, "Head Dot");
			ImGui::Toggle(box_esp.decrypt(), &Esp_box);
			ImGui::Toggle(corner_esp.decrypt(), &cornerbox);
			ImGui::Toggle(snapline.decrypt(), &Esp_line);
			ImGui::Toggle(aemline.decrypt(), &aimline);
			ImGui::Toggle(heddot.decrypt(), &headdot);
			ImGui::Text(es5.decrypt());
			ImGui::SliderInt("", &VisDist, 50, 270);
		}

		if (maintabs == 2)
		{
			XorS(streamproof_esp, "Streamproof");
			ImGui::Toggle(streamproof_esp.decrypt(), &streamproof);
		}
		

	
		

		ImGui::End();
	}

	DrawESP();

	ImGui::EndFrame();
	D3dDevice->SetRenderState(D3DRS_ZENABLE, false);
	D3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
	D3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, false);
	D3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);

	if (D3dDevice->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		D3dDevice->EndScene();
	}
	HRESULT result = D3dDevice->Present(NULL, NULL, NULL, NULL);

	if (result == D3DERR_DEVICELOST && D3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
	{
		ImGui_ImplDX9_InvalidateDeviceObjects();
		D3dDevice->Reset(&d3dpp);
		ImGui_ImplDX9_CreateDeviceObjects();
	}
}

MSG Message = { NULL };
int Loop = 0;
void xMainLoop()
{
	static RECT old_rc;
	ZeroMemory(&Message, sizeof(MSG));

	while (Message.message != WM_QUIT)
	{
		if (PeekMessage(&Message, Window, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}

		HWND hwnd_active = GetForegroundWindow();

		if (hwnd_active == hwnd) {
			HWND hwndtest = GetWindow(hwnd_active, GW_HWNDPREV);
			SetWindowPos(Window, hwndtest, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}

		if (GetAsyncKeyState(0x23) & 1)
			exit(8);

		RECT rc;
		POINT xy;

		ZeroMemory(&rc, sizeof(RECT));
		ZeroMemory(&xy, sizeof(POINT));
		GetClientRect(hwnd, &rc);
		ClientToScreen(hwnd, &xy);
		rc.left = xy.x;
		rc.top = xy.y;

		ImGuiIO& io = ImGui::GetIO();
		io.IniFilename = NULL;
		io.ImeWindowHandle = hwnd;
		io.DeltaTime = 1.0f / 60.0f;

		POINT p;
		GetCursorPos(&p);
		io.MousePos.x = p.x - xy.x;
		io.MousePos.y = p.y - xy.y;

		if (GetAsyncKeyState(VK_LBUTTON)) {
			io.MouseDown[0] = true;
			io.MouseClicked[0] = true;
			io.MouseClickedPos[0].x = io.MousePos.x;
			io.MouseClickedPos[0].x = io.MousePos.y;
		}
		else
			io.MouseDown[0] = false;

		if (rc.left != old_rc.left || rc.right != old_rc.right || rc.top != old_rc.top || rc.bottom != old_rc.bottom)
		{
			old_rc = rc;

			Width = rc.right;
			Height = rc.bottom;

			d3dpp.BackBufferWidth = Width;
			d3dpp.BackBufferHeight = Height;
			SetWindowPos(Window, (HWND)0, xy.x, xy.y, Width, Height, SWP_NOREDRAW);
			D3dDevice->Reset(&d3dpp);
		}
		render();
		if (Loop == 0) {
			if (Uworld) {
				//printf("[+] Uworld Called\n");
			}
			else if (!Uworld)
			{
				//printf("[+] Uworld Failed To Be Called\n");
			}
			if (PlayerController) {
				//printf("[+] Player Controller Called\n");
			}
			else if (!PlayerController)
			{
				//printf("[+] Player Controller Failed To Be Called\n");
			}
			if (Rootcomp) {
				//printf("[+] Root Component Called\n");
			}
			else if (!Rootcomp)
			{
				//printf("[+] Root Component Failed To Be Called\n");
			}
			if (PlayerState) {
				//printf("[+] Player State Called\n");
			}
			else if (!PlayerState)
			{
				//printf("\n[+] Player State Failed To Be Called\n");
			}
		}
		Loop = Loop + 1;
	}
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	DestroyWindow(Window);
}

LRESULT CALLBACK WinProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, Message, wParam, lParam))
		return true;

	switch (Message)
	{
	case WM_DESTROY:
		xShutdown();
		PostQuitMessage(0);
		exit(4);
		break;
	case WM_SIZE:
		if (D3dDevice != NULL && wParam != SIZE_MINIMIZED)
		{
			ImGui_ImplDX9_InvalidateDeviceObjects();
			d3dpp.BackBufferWidth = LOWORD(lParam);
			d3dpp.BackBufferHeight = HIWORD(lParam);
			HRESULT hr = D3dDevice->Reset(&d3dpp);
			if (hr == D3DERR_INVALIDCALL)
				IM_ASSERT(0);
			ImGui_ImplDX9_CreateDeviceObjects();
		}
		break;
	default:
		return DefWindowProc(hWnd, Message, wParam, lParam);
		break;
	}
	return 0;
}

void xShutdown()
{
	TriBuf->Release();
	D3dDevice->Release();
	p_Object->Release();

	DestroyWindow(Window);
	UnregisterClass("fgers", NULL);
}
