#include "StriveHitboxes.h"
#include "Utilities/MinHook.h"
#include "sigscan.h"
#include "arcsys.h"
#include "math_util.h"
#include <bit>
#include <array>
#undef min
#undef max

#define PURE_VIRTUAL(func,...) =0;

enum
{
	// Default allocator alignment. If the default is specified, the allocator applies to engine rules.
	// Blocks >= 16 bytes will be 16-byte-aligned, Blocks < 16 will be 8-byte aligned. If the allocator does
	// not support allocation alignment, the alignment will be ignored.
	DEFAULT_ALIGNMENT = 0,

	// Minimum allocator alignment
	MIN_ALIGNMENT = 8,
};

template<typename Fn>
inline Fn GetVFunc(const void* v, int i)
{
	std::cout << *(const void***)v << std::endl;
	return (Fn) * (*(const void***)v + i);
}

typedef void(*FMemory_Free)(void* Original);
FMemory_Free Free;

inline bool FileExists(std::string& name) {
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}

struct FGenericMemoryStats;

class FOutputDevice;

class FExec
{
public:
	virtual ~FExec();
	virtual bool Exec(UE4::UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) PURE_VIRTUAL(FExec::Exec, return false;)
};

class FMalloc : public FExec
{
public:
	virtual void* Malloc(size_t Count, uint32_t Alignment = DEFAULT_ALIGNMENT) = 0;

	virtual void* TryMalloc(size_t Count, uint32_t Alignment = DEFAULT_ALIGNMENT);

	virtual void* Realloc(void* Original, size_t Count, uint32_t Alignment = DEFAULT_ALIGNMENT) = 0;

	virtual void* TryRealloc(void* Original, size_t Count, uint32_t Alignment = DEFAULT_ALIGNMENT);

	virtual void Free(void* Original) = 0;

	virtual size_t QuantizeSize(size_t Count, uint32_t Alignment)
	{
		return Count; // Default implementation has no way of determining this
	}

	virtual bool GetAllocationSize(void* Original, size_t& SizeOut)
	{
		return false; // Default implementation has no way of determining this
	}

	virtual void Trim(bool bTrimThreadCaches)
	{
	}

	virtual void SetupTLSCachesOnCurrentThread()
	{
	}

	virtual void ClearAndDisableTLSCachesOnCurrentThread()
	{
	}

	virtual void InitializeStatsMetadata();

	virtual bool Exec(UE4::UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override
	{
		return false;
	}

	virtual void UpdateStats();

	virtual void GetAllocatorStats(FGenericMemoryStats& out_Stats);

	virtual void DumpAllocatorStats(class FOutputDevice& Ar);

	virtual bool IsInternallyThreadSafe() const
	{
		return false;
	}

	virtual bool ValidateHeap()
	{
		return(true);
	}

	virtual const TCHAR* GetDescriptiveName()
	{
		return "Unspecified allocator";
	}
};

FMalloc* GMalloc;


struct FCanvasUVTri {
	FVector2D V0_Pos;
	FVector2D V0_UV;
	UE4::FLinearColor V0_Color;
	FVector2D V1_Pos;
	FVector2D V1_UV;
	UE4::FLinearColor V1_Color;
	FVector2D V2_Pos;
	FVector2D V2_UV;
	UE4::FLinearColor V2_Color;
};

class FCanvas {
public:
	enum ECanvasDrawMode {
		CDM_DeferDrawing,
		CDM_ImmediateDrawing
	};

	FIELD(0xA0, ECanvasDrawMode, DrawMode);

	void Flush_GameThread(bool bForce = false);
};

class UTexture;

class UCanvas : public UE4::UObject {
public:
	void K2_DrawLine(FVector2D ScreenPositionA, FVector2D ScreenPositionB, float Thickness, UE4::FLinearColor RenderColor);
	UE4::FVector K2_Project(const UE4::FVector WorldPosition);

	void K2_DrawTriangle(UTexture* RenderTexture, UE4::TArray<FCanvasUVTri>* Triangles);

	FIELD(0x260, FCanvas*, Canvas);
};

using UCanvas_K2_DrawLine_t = void(*)(UCanvas*, FVector2D, FVector2D, float, const UE4::FLinearColor&);
const auto UCanvas_K2_DrawLine = (UCanvas_K2_DrawLine_t)(
	sigscan::get().scan("\x0F\x2F\xC8\x0F\x86\x94", "xxxxxx") - 0x51);

using UCanvas_K2_Project_t = void(*)(const UCanvas*, UE4::FVector*, const UE4::FVector&);
const auto UCanvas_K2_Project = (UCanvas_K2_Project_t)(
	sigscan::get().scan("\x48\x8B\x89\x68\x02\x00\x00\x48\x8B\xDA", "xxxxxxxxxx") - 0x12);

using UCanvas_K2_DrawTriangle_t = void(*)(UCanvas*, UTexture*, UE4::TArray<FCanvasUVTri>*);
const auto UCanvas_K2_DrawTriangle = (UCanvas_K2_DrawTriangle_t)(
	sigscan::get().scan("\x48\x81\xEC\x90\x00\x00\x00\x41\x83\x78\x08\x00", "xxxxxxxxxxxx") - 6);

void UCanvas::K2_DrawLine(FVector2D ScreenPositionA, FVector2D ScreenPositionB, float Thickness, UE4::FLinearColor RenderColor)
{
	UCanvas_K2_DrawLine(this, ScreenPositionA, ScreenPositionB, Thickness, RenderColor);
}

UE4::FVector UCanvas::K2_Project(const UE4::FVector WorldPosition)
{
	UE4::FVector out;
	UCanvas_K2_Project(this, &out, WorldPosition);
	return out;
}

void UCanvas::K2_DrawTriangle(UTexture* RenderTexture, UE4::TArray<FCanvasUVTri>* Triangles)
{
	UCanvas_K2_DrawTriangle(this, RenderTexture, Triangles);
}

class AHUD : public UE4::UObject {
public:
	FIELD(0x278, UCanvas*, Canvas);
};

constexpr auto AHUD_PostRender_index = 214;

// Actually AREDHUD_Battle
const auto** AHUD_vtable = (const void**)get_rip_relative(sigscan::get().scan(
	"\x48\x8D\x05\x00\x00\x00\x00\xC6\x83\x18\x03", "xxx????xxxx") + 3);

using AHUD_PostRender_t = void(*)(AHUD*);
AHUD_PostRender_t orig_AHUD_PostRender;

class FTexture** GWhiteTexture = (FTexture**)get_rip_relative((uintptr_t)UCanvas_K2_DrawTriangle + 0x3A);;

enum ESimpleElementBlendMode
{
	SE_BLEND_Opaque = 0,
	SE_BLEND_Masked,
	SE_BLEND_Translucent,
	SE_BLEND_Additive,
	SE_BLEND_Modulate,
	SE_BLEND_MaskedDistanceField,
	SE_BLEND_MaskedDistanceFieldShadowed,
	SE_BLEND_TranslucentDistanceField,
	SE_BLEND_TranslucentDistanceFieldShadowed,
	SE_BLEND_AlphaComposite,
	SE_BLEND_AlphaHoldout,
	// Like SE_BLEND_Translucent, but modifies destination alpha
	SE_BLEND_AlphaBlend,
	// Like SE_BLEND_Translucent, but reads from an alpha-only texture
	SE_BLEND_TranslucentAlphaOnly,
	SE_BLEND_TranslucentAlphaOnlyWriteAlpha,

	SE_BLEND_RGBA_MASK_START,
	SE_BLEND_RGBA_MASK_END = SE_BLEND_RGBA_MASK_START + 31, //Using 5bit bit-field for red, green, blue, alpha and desaturation

	SE_BLEND_MAX
};

class FCanvasItem {
protected:
	FCanvasItem() = default;

public:

	// Virtual function wrapper
	void Draw(class FCanvas* InCanvas)
	{
		using Draw_t = void(*)(FCanvasItem*, class FCanvas*);
		((Draw_t)(*(void***)this)[3])(this, InCanvas);
	}

	FIELD(0x14, ESimpleElementBlendMode, BlendMode);
};

class FCanvasTriangleItem : public FCanvasItem {
public:
	FCanvasTriangleItem(
		const FVector2D& InPointA,
		const FVector2D& InPointB,
		const FVector2D& InPointC,
		const FTexture* InTexture);

	~FCanvasTriangleItem()
	{
		TriangleList.~TArray<FCanvasUVTri>();
	}

	FIELD(0x50, UE4::TArray<FCanvasUVTri>, TriangleList);

private:
	char pad[0x60];
};

static_assert(sizeof(FCanvasTriangleItem) == 0x60);

using FCanvasTriangleItem_ctor_t = void(*)(
	FCanvasTriangleItem*,
	const FVector2D&,
	const FVector2D&,
	const FVector2D&,
	const FTexture*);
const auto FCanvasTriangleItem_ctor = (FCanvasTriangleItem_ctor_t)(
	sigscan::get().scan("\x48\x89\x4B\x10\x88\x4B\x18", "xxxxxxx") - 0x34);

FCanvasTriangleItem::FCanvasTriangleItem(
	const FVector2D& InPointA,
	const FVector2D& InPointB,
	const FVector2D& InPointC,
	const FTexture* InTexture)
{
	FCanvasTriangleItem_ctor(this, InPointA, InPointB, InPointC, InTexture);
}
struct drawn_hitbox {
	hitbox::box_type type;

	// Unclipped corners of filled box
	std::array<FVector2D, 4> corners;

	// Boxes to fill, clipped against other boxes
	std::vector<std::array<FVector2D, 4>> fill;

	// Outlines
	std::vector<std::array<FVector2D, 2>> lines;

	drawn_hitbox(const hitbox& box) :
		type(box.type),
		corners{
			FVector2D(box.x, box.y),
			FVector2D(box.x + box.w, box.y),
			FVector2D(box.x + box.w, box.y + box.h),
			FVector2D(box.x, box.y + box.h) }
	{
		for (auto i = 0; i < 4; i++)
			lines.push_back(std::array{ corners[i], corners[(i + 1) % 4] });

		fill.push_back(corners);
	}

	// Clip outlines against another hitbox
	void clip_lines(const drawn_hitbox& other)
	{
		auto old_lines = std::move(lines);
		lines.clear();

		for (auto& line : old_lines) {
			float entry_fraction, exit_fraction;
			auto intersected = line_box_intersection(
				other.corners[0], other.corners[2],
				line[0], line[1],
				&entry_fraction, &exit_fraction);

			if (!intersected) {
				lines.push_back(line);
				continue;
			}

			const auto delta = line[1] - line[0];

			if (entry_fraction != 0.f)
				lines.push_back(std::array{ line[0], line[0] + delta * entry_fraction });

			if (exit_fraction != 1.f)
				lines.push_back(std::array{ line[0] + delta * exit_fraction, line[1] });
		}
	}

	// Clip filled rectangle against another hitbox
	void clip_fill(const drawn_hitbox& other)
	{
		auto old_fill = std::move(fill);
		fill.clear();

		for (const auto& box : old_fill) {
			const auto& box_min = box[0];
			const auto& box_max = box[2];

			const auto clip_min = FVector2D(
				max(box_min.X, other.corners[0].X),
				max(box_min.Y, other.corners[0].Y));

			const auto clip_max = FVector2D(
				min(box_max.X, other.corners[2].X),
				min(box_max.Y, other.corners[2].Y));

			if (clip_min.X > clip_max.X || clip_min.Y > clip_max.Y) {
				// No intersection
				fill.push_back(box);
				continue;
			}

			if (clip_min.X > box_min.X) {
				// Left box
				fill.push_back(std::array{
					FVector2D(box_min.X, box_min.Y),
					FVector2D(clip_min.X, box_min.Y),
					FVector2D(clip_min.X, box_max.Y),
					FVector2D(box_min.X, box_max.Y) });
			}

			if (clip_max.X < box_max.X) {
				// Right box
				fill.push_back(std::array{
					FVector2D(clip_max.X, box_min.Y),
					FVector2D(box_max.X, box_min.Y),
					FVector2D(box_max.X, box_max.Y),
					FVector2D(clip_max.X, box_max.Y) });
			}

			if (clip_min.Y > box_min.Y) {
				// Top box
				fill.push_back(std::array{
					FVector2D(clip_min.X, box_min.Y),
					FVector2D(clip_max.X, box_min.Y),
					FVector2D(clip_max.X, clip_min.Y),
					FVector2D(clip_min.X, clip_min.Y) });
			}

			if (clip_max.Y < box_max.Y) {
				// Bottom box
				fill.push_back(std::array{
					FVector2D(clip_min.X, clip_max.Y),
					FVector2D(clip_max.X, clip_max.Y),
					FVector2D(clip_max.X, box_max.Y),
					FVector2D(clip_min.X, box_max.Y) });
			}
		}
	}
};

void asw_coords_to_screen(UCanvas* canvas, FVector2D* pos)
{
	pos->X *= asw_engine::COORD_SCALE / 1000.F;
	pos->Y *= asw_engine::COORD_SCALE / 1000.F;

	UE4::FVector pos3d(pos->X, 0.f, pos->Y);
	asw_scene::get()->camera_transform(&pos3d, nullptr);

	const auto proj = canvas->K2_Project(pos3d);
	*pos = FVector2D(proj.X, proj.Y);
}

// Corners must be in CW or CCW order
void fill_rect(
	UCanvas* canvas,
	const std::array<FVector2D, 4>& corners,
	const UE4::FLinearColor& color)
{
	FCanvasUVTri triangles[2];
	triangles[0].V0_Color = triangles[0].V1_Color = triangles[0].V2_Color = color;
	triangles[1].V0_Color = triangles[1].V1_Color = triangles[1].V2_Color = color;

	triangles[0].V0_Pos = corners[0];
	triangles[0].V1_Pos = corners[1];
	triangles[0].V2_Pos = corners[2];

	triangles[1].V0_Pos = corners[2];
	triangles[1].V1_Pos = corners[3];
	triangles[1].V2_Pos = corners[0];

	FCanvasTriangleItem item(
		FVector2D(0.f, 0.f),
		FVector2D(0.f, 0.f),
		FVector2D(0.f, 0.f),
		*GWhiteTexture);

	UE4::TArray<FCanvasUVTri> List;
	for (auto Triangle : triangles)
		List.Add(Triangle);

	item.TriangleList = List;
	item.BlendMode = SE_BLEND_Translucent;
	item.Draw(canvas->Canvas);
}

// Corners must be in CW or CCW order
void draw_rect(
	UCanvas* canvas,
	const std::array<FVector2D, 4>& corners,
	const UE4::FLinearColor& color)
{
	fill_rect(canvas, corners, color);

	for (auto i = 0; i < 4; i++)
		canvas->K2_DrawLine(corners[i], corners[(i + 1) % 4], 2.F, color);
}

#define M_PI   3.14159265358979323846264338327950288

// Transform entity local space to screen space
void transform_hitbox_point(UCanvas* canvas, const asw_entity* entity, FVector2D* pos, bool is_throw)
{
	if (!is_throw) {
		pos->X *= -entity->scale_x;
		pos->Y *= -entity->scale_y;

		*pos = pos->Rotate((float)entity->angle_x * (float)M_PI / 180000.f);

		if (entity->facing == direction::left)
			pos->X *= -1.f;
	}
	else if (entity->opponent != nullptr) {
		// Throws hit on either side, so show it directed towards opponent
		if (entity->get_pos_x() > entity->opponent->get_pos_x())
			pos->X *= -1.f;
	}

	pos->X += entity->get_pos_x();
	pos->Y += entity->get_pos_y();

	asw_coords_to_screen(canvas, pos);
}

void draw_hitbox(UCanvas* canvas, const asw_entity* entity, const drawn_hitbox& box)
{
	UE4::FLinearColor color;
	if (box.type == hitbox::box_type::hit)
		color = UE4::FLinearColor(1.f, 0.f, 0.f, .25f);
	else if (box.type == hitbox::box_type::grab)
		color = UE4::FLinearColor(1.f, 0.f, 1.f, .25f);
	else if (entity->counterhit)
		color = UE4::FLinearColor(0.f, 1.f, 1.f, .25f);
	else
		color = UE4::FLinearColor(0.f, 1.f, 0.f, .25f);

	const auto is_throw = box.type == hitbox::box_type::grab;

	for (auto fill : box.fill) {
		for (auto& pos : fill)
			transform_hitbox_point(canvas, entity, &pos, is_throw);

		fill_rect(canvas, fill, color);
	}

	for (const auto& line : box.lines) {
		auto start = line[0];
		auto end = line[1];
		transform_hitbox_point(canvas, entity, &start, is_throw);
		transform_hitbox_point(canvas, entity, &end, is_throw);
		canvas->K2_DrawLine(start, end, 2.F, color);
	}
}

hitbox calc_throw_box(const asw_entity* entity)
{
	// Create a fake hitbox for throws to be displayed
	hitbox box;
	box.type = hitbox::box_type::grab;

	const auto pushbox_front = entity->pushbox_width() / 2 + entity->pushbox_front_offset;
	box.x = 0.f;
	box.w = (float)(pushbox_front + entity->throw_range);

	if (entity->throw_box_top <= entity->throw_box_bottom) {
		// No throw height, use pushbox height for display
		box.y = 0.f;
		box.h = (float)entity->pushbox_height();
		return box;
	}

	box.y = (float)entity->throw_box_bottom;
	box.h = (float)(entity->throw_box_top - entity->throw_box_bottom);
	return box;
}

void draw_hitboxes(UCanvas* canvas, const asw_entity* entity, bool active)
{
	const auto count = entity->hitbox_count + entity->hurtbox_count;

	std::vector<drawn_hitbox> hitboxes;

	// Collect hitbox info
	for (auto i = 0; i < count; i++) {
		const auto& box = entity->hitboxes[i];

		// Don't show inactive hitboxes
		if (box.type == hitbox::box_type::hit && !active)
			continue;
		else if (box.type == hitbox::box_type::hurt && entity->is_strike_invuln())
			continue;

		hitboxes.push_back(drawn_hitbox(box));
	}

	// Add throw hitbox if in use
	if (entity->throw_range >= 0 && active)
		hitboxes.push_back(calc_throw_box(entity));

	for (auto i = 0; i < hitboxes.size(); i++) {
		// Clip outlines
		for (auto j = 0; j < hitboxes.size(); j++) {
			if (i != j && hitboxes[i].type == hitboxes[j].type)
				hitboxes[i].clip_lines(hitboxes[j]);
		}

		// Clip fill against every hitbox after, since two boxes
		// shouldn't both be clipped against each other
		for (auto j = i + 1; j < hitboxes.size(); j++) {
			if (hitboxes[i].type == hitboxes[j].type)
				hitboxes[i].clip_fill(hitboxes[j]);
		}

		draw_hitbox(canvas, entity, hitboxes[i]);
	}
}

void draw_pushbox(UCanvas* canvas, const asw_entity* entity)
{
	int left, top, right, bottom;
	entity->get_pushbox(&left, &top, &right, &bottom);

	std::array corners = {
		FVector2D(left, top),
		FVector2D(right, top),
		FVector2D(right, bottom),
		FVector2D(left, bottom)
	};

	for (auto& pos : corners)
		asw_coords_to_screen(canvas, &pos);

	// Show hollow pushbox when throw invuln
	UE4::FLinearColor color;
	if (entity->is_throw_invuln())
		color = UE4::FLinearColor(1.f, 1.f, 0.f, 0.f);
	else
		color = UE4::FLinearColor(1.f, 1.f, 0.f, .2f);

	draw_rect(canvas, corners, color);
}

void draw_display(UCanvas* canvas)
{
	const auto* engine = asw_engine::get();
	if (engine == nullptr)
		return;

	if (canvas->Canvas == nullptr)
		return;

	// Loop through entities backwards because the player that most
	// recently landed a hit is at index 0
	for (auto entidx = engine->entity_count - 1; entidx >= 0; entidx--) {
		const auto* entity = engine->entities[entidx];

		if (entity->is_pushbox_active())
			draw_pushbox(canvas, entity);

		const auto active = entity->is_active();
		draw_hitboxes(canvas, entity, active);

		const auto* attached = entity->attached;
		while (attached != nullptr) {
			draw_hitboxes(canvas, attached, active);
			attached = attached->attached;
		}
	}
}

void hook_AHUD_PostRender(AHUD* hud)
{
	draw_display(hud->Canvas);
	orig_AHUD_PostRender(hud);
}

const void* vtable_hook(const void** vtable, const int index, const void* hook)
{
	DWORD old_protect;
	VirtualProtect(&vtable[index], sizeof(void*), PAGE_READWRITE, &old_protect);
	const auto* orig = vtable[index];
	vtable[index] = hook;
	VirtualProtect(&vtable[index], sizeof(void*), old_protect, &old_protect);
	return orig;
}

void install_hooks()
{
	// AHUD::PostRender
	orig_AHUD_PostRender = (AHUD_PostRender_t)
		vtable_hook(AHUD_vtable, AHUD_PostRender_index, hook_AHUD_PostRender);
}

bool ShouldUpdateBattle = true;

typedef void(*UpdateBattle_Func)(AREDGameState_Battle*, float, bool);
UpdateBattle_Func UpdateBattle;

enum GAME_MODE : int32_t
{
	GAME_MODE_DEBUG_BATTLE = 0x0,
	GAME_MODE_ADVERTISE = 0x1,
	GAME_MODE_MAINTENANCEVS = 0x2,
	GAME_MODE_ARCADE = 0x3,
	GAME_MODE_MOM = 0x4,
	GAME_MODE_SPARRING = 0x5,
	GAME_MODE_VERSUS = 0x6,
	GAME_MODE_VERSUS_PREINSTALL = 0x7,
	GAME_MODE_TRAINING = 0x8,
	GAME_MODE_TOURNAMENT = 0x9,
	GAME_MODE_RANNYU_VERSUS = 0xA,
	GAME_MODE_EVENT = 0xB,
	GAME_MODE_SURVIVAL = 0xC,
	GAME_MODE_STORY = 0xD,
	GAME_MODE_MAINMENU = 0xE,
	GAME_MODE_TUTORIAL = 0xF,
	GAME_MODE_LOBBYTUTORIAL = 0x10,
	GAME_MODE_CHALLENGE = 0x11,
	GAME_MODE_KENTEI = 0x12,
	GAME_MODE_MISSION = 0x13,
	GAME_MODE_GALLERY = 0x14,
	GAME_MODE_LIBRARY = 0x15,
	GAME_MODE_NETWORK = 0x16,
	GAME_MODE_REPLAY = 0x17,
	GAME_MODE_LOBBYSUB = 0x18,
	GAME_MODE_MAINMENU_QUICK_BATTLE = 0x19,
	GAME_MODE_UNDECIDED = 0x1A,
	GAME_MODE_INVALID = 0x1B,
};

class UREDGameCommon : public UE4::UGameInstance {};
UREDGameCommon* GameCommon;

typedef int(*GetGameMode_Func)(UREDGameCommon*);
GetGameMode_Func GetGameMode;

void(*UpdateBattle_Orig)(AREDGameState_Battle*, float, bool);
void UpdateBattle_New(AREDGameState_Battle* GameState, float DeltaTime, bool bUpdateDraw) {
	if (ShouldUpdateBattle || GetGameMode(GameCommon) != (int)GAME_MODE_TRAINING)
		UpdateBattle_Orig(GameState, DeltaTime, bUpdateDraw);
}

BPFUNCTION(ToggleUpdateBattle)
{
	ShouldUpdateBattle = !ShouldUpdateBattle;
}

BPFUNCTION(AdvanceBattle)
{
	if (ShouldUpdateBattle || GetGameMode(GameCommon) != (int)GAME_MODE_TRAINING) return;
	UE4::UClass* Class = UE4::UObject::FindClass("Class RED.REDGameState_Battle");
	if (*UE4::UWorld::GWorld == nullptr)
		return;

	const auto* GameState = ((UWorld*)(*UE4::UWorld::GWorld))->GameState;

	if (!GameState->IsA(Class))
		return;

	UpdateBattle_Orig((AREDGameState_Battle*)GameState, 0.01666666f, true);
}

// Only Called Once, if you need to hook shit, declare some global non changing values
void StriveHitboxes::InitializeMod()
{
	UE4::InitSDK();
	SetupHooks();

	REGISTER_FUNCTION(ToggleUpdateBattle);
	REGISTER_FUNCTION(AdvanceBattle);

	HMODULE BaseModule = GetModuleHandleW(NULL);
	uint64_t* FMemory_Free_Addr = (uint64_t*)sigscan::get().scan("\x48\x85\xC9\x74\x2E\x53\x48\x83\xEC\x20\x48\x8B\xD9\x48\x8B\x00\x00\x00\x00\x00\x48\x85\xC9", "xxxxxxxxxxxxxxx?????xxx");
	Free = (FMemory_Free)FMemory_Free_Addr;

	// 13 (0xD) bytes into FMemory::Free is a MOV instruction
	// 'GMalloc' gets MOV'd into some register
	// The following code resolves the MOV instruction

	uint8_t* mov_instruction = (uint8_t*)FMemory_Free_Addr + 0xD;
	// Instruction size, including REX and ModR
	constexpr uint8_t instr_size = 0x7;
	uint8_t* next_instruction = mov_instruction + instr_size;
	uint32_t* offset = std::bit_cast<uint32_t*>(mov_instruction + 0x3);
	GMalloc = *std::bit_cast<FMalloc**>(next_instruction + *offset);

	MinHook::Init(); //Uncomment if you plan to do hooks

	uintptr_t UpdateBattle_Addr = (uintptr_t)sigscan::get().scan("\x45\x33\xF6\x0F\xB6\xD8", "xxxxxx") - 0x32;
	UpdateBattle = (UpdateBattle_Func)UpdateBattle_Addr;
	MinHook::Add((DWORD_PTR)UpdateBattle, &UpdateBattle_New, &UpdateBattle_Orig, "UpdateBattle");

	uintptr_t GetGameMode_Addr = (uintptr_t)sigscan::get().scan("\x0F\xB6\x81\xF0\x02\x00\x00\xC3", "xxxxxxxx");
	GetGameMode = (GetGameMode_Func)GetGameMode_Addr;

	install_hooks();

	GameCommon = (UREDGameCommon*)UE4::UGameplayStatics::GetGameInstance();

	//UseMenuButton = true; // Allows Mod Loader To Show Button
}

void StriveHitboxes::InitGameState()
{
}

void StriveHitboxes::BeginPlay(UE4::AActor* Actor)
{
}

void StriveHitboxes::PostBeginPlay(std::wstring ModActorName, UE4::AActor* Actor)
{
	// Filters Out All Mod Actors Not Related To Your Mod
	std::wstring TmpModName(ModName.begin(), ModName.end());
	if (ModActorName == TmpModName)
	{
		//Sets ModActor Ref
		ModActor = Actor;
	}
}

void StriveHitboxes::DX11Present(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, ID3D11RenderTargetView* pRenderTargetView)
{
}

void StriveHitboxes::OnModMenuButtonPressed()
{
}

void StriveHitboxes::DrawImGui()
{
}