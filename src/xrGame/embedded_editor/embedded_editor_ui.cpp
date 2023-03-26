#include "stdafx.h"
#include "embedded_editor_ui.h"
#include "HUDmanager.h"
#include "Level.h"
#include "UIGameCustom.h"
#include "embedded_editor_helper.h"
#include "ui/UIFrameWindow.h"
#include "ui/UIMainIngameWnd.h"
#include "ui/UITextureMaster.h"
#include "ui/uidialogwnd.h"
#include "ui/UIStatic.h"
#include <Shlwapi.h>

xr_map<CUIWindow*, bool> collapsed;
CUIWindow* curWnd = nullptr;

// èìÿ òèïà âîçâðàùàåòñÿ âèäà "class XXX", îòáðàñûâàþ ïåðâûå 6 ñèìâîëîâ
const char* getType(CUIWindow* w) { return typeid(*w).name() + 6; }

void showMenu(CUIWindow* w)
{
    if (ImGui::BeginMenu("Add")) {
        ImGui::MenuItem("CUIWindow");
        ImGui::MenuItem("CUIStatic");
        ImGui::MenuItem("CUIFrameWindow");
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Delete")) {
    }
}

void wndHandler(CUIWindow* w)
{
    bool isCollapsed = collapsed[w];
    ImGui::PushID(w);
    if (w->GetChildNum() != 0) {
        if (ImGui::Button(isCollapsed ? ">##col" : "v##col"))
            collapsed[w] = !isCollapsed;
        ImGui::SameLine();
    }
    if (ImGui::Button(w->GetVisible() ? "o##vis" : "-##vis"))
        w->SetVisible(!w->GetVisible());
    ImGui::SameLine();
    char name[100];
    strconcat(100, name, w->WindowName().data(), ": ", getType(w));
    if (ImGui::Selectable(name, w == curWnd))
        curWnd = w;
    if (ImGui::BeginPopupContextItem("")) {
        showMenu(w);
        ImGui::EndPopup();
    }
    ImGui::PopID();
    if (isCollapsed)
        return;
    ImGui::Indent(20.0f);
    for (auto&& el : w->GetChildWndList())
        wndHandler(el);
    ImGui::Unindent(20.0f);
};

void showWndList(bool& show)
{
    ImguiWnd wnd("UI Browser", &show, ImGuiWindowFlags_MenuBar);
    if (wnd.Collapsed)
        return;

    if (ImGui::BeginMenuBar()) {
        showMenu(curWnd);
        ImGui::EndMenuBar();
    }

    CUIWindow* w = CurrentGameUI()->TopInputReceiver();
    if (!w)
        w = CurrentGameUI()->UIMainIngameWnd;
    if (!w)
        return;

    wndHandler(w);
    ImGui::Indent(0.0f);
}

ImColor toImColor(u32 bgra)
{
    // u8 b = bgra >> 24;
    // u8 r = (bgra >> 8) & 0xFF;
    // return ImColor((bgra & 0x00FF00FF) | (r << 24) | (b << 8));
    return bgra;
}

u32 fromImColor(const ImColor& c)
{
    u32 abgr = (ImU32)c;
    // u8 r = rgba >> 24;
    // u8 b = (rgba >> 8) & 0xFF;
    // return (rgba & 0x00FF00FF) | (b << 24) | (r << 8);
    return abgr;
}

//extern bool ImGui_ListBox(const char* label, int* current_item, bool (*items_getter)(void*, int, const char**), void* data,
    //int items_count, const ImVec2& size_arg = ImVec2(0, 0));

bool editTexture(shared_str& texName)
{
    char tex[100];
    strncpy(tex, texName.data(), 100);
    bool changed = false;
    static shared_str prevValue;
    ImGui::Text("Texture");
    ImGui::SameLine();
    if (ImGui::InputText("##texture", tex, 100)) {
        texName = tex;
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("...##texture")) {
        ImGui::OpenPopup("Choose texture");
        prevValue = texName;
    }
    if (ImGui::BeginPopupModal("Choose texture", NULL, 0)) {
        static xr_vector<xr_string> list = CUITextureMaster::GetTextureList();
        static char filter[100] = {};
        if (ImGui::InputText("##filter", filter, 100)) {
            auto fullList = CUITextureMaster::GetTextureList();
            const char* p = filter;
			if (p[0] != '\0') {
				auto e = std::copy_if(fullList.begin(), fullList.end(), list.begin(),
					[p](const auto& x) { return strstr(x.c_str(), p); });
				list.resize(e - list.begin());
			}
			else
				list = fullList;
        }
        auto it = std::lower_bound(list.begin(), list.end(), texName.data());
        int cur = (it != list.end()) ? (it - list.begin()) : -1;
        if (ImGui_ListBox("", &cur,
                [](void* data, int idx, const char** out_text) -> bool {
                    xr_vector<xr_string>* textures = (xr_vector<xr_string>*)data;
                    *out_text = (*textures)[idx].c_str();
                    return true;
                },
                &list, list.size(), ImVec2(-1.0f, -50.0f))) {
        }
        ImGui::SameLine();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            texName = list[cur].c_str();
            changed = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            texName = prevValue;
            changed = true;
        }
        ImGui::EndPopup();
    }
    return changed;
}

void showWndProp(bool& show)
{
    ImguiWnd wnd("Window Properties", &show);
    if (wnd.Collapsed)
        return;
    if (!curWnd)
        return;
    xr_string type = getType(curWnd);
    ImGui::Text(type.c_str());
    if (ImGui::CollapsingHeader("CUIWindow")) {
        Frect r = curWnd->GetWndRect();
        float width = r.width();
        float height = r.height();
        if (ImGui::InputFloat("Left", &r.left, 1.0f, 10.0f)) {
            r.right = r.left + width;
            curWnd->SetWndRect(r);
        }
        if (ImGui::InputFloat("Top", &r.top, 1.0f, 10.0f)) {
            r.bottom = r.top + height;
            curWnd->SetWndRect(r);
        }
        if (ImGui::InputFloat("Width", &width, 1.0f, 10.0f)) {
            r.right = r.left + width;
            curWnd->SetWndRect(r);
        }
        if (ImGui::InputFloat("Height", &height, 1.0f, 10.0f)) {
            r.bottom = r.top + height;
            curWnd->SetWndRect(r);
        }
        char name[100];
        auto n = curWnd->WindowName();
        strncpy(name, n.data(), 100);
        if (ImGui::InputText("Name", name, 100))
            curWnd->SetWindowName(name);
    }
    if (type == "CUITextWnd" && ImGui::CollapsingHeader("CUITextWnd")) {
        CUITextWnd* s = smart_cast<CUITextWnd*>(curWnd);
        char text[100];
        auto t = s->GetText();
        strncpy(text, t, 100);
        if (ImGui::InputText("Text", text, 100))
            s->SetText(text);
        ImColor tempColor = toImColor(s->GetTextColor());
        if (ImGui::ColorEdit4("Text color", (float*)&tempColor, ImGuiColorEditFlags_AlphaBar))
            s->SetTextColor(fromImColor(tempColor));
        //"Text offset";
    }
    if (type == "CUIFrameWindow" && ImGui::CollapsingHeader("CUIFrameWindow")) {
        CUIFrameWindow* f = dynamic_cast<CUIFrameWindow*>(curWnd);
        shared_str tex = f->TextureName();
        if (editTexture(tex))
            f->InitTexture(tex.data());
    }
}

void showWndHud()
{
    if (!curWnd)
        return;

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2());
    bool open = true;
	ImGui::SetNextWindowBgAlpha(0.0f);
	ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("ALL_SCREEN", &open, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing
		| ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PushClipRect(ImVec2(), io.DisplaySize, false);

    Frect r;
    curWnd->GetAbsoluteRect(r);
    UI().ClientToScreenScaled(r.lt, r.lt.x, r.lt.y);
    UI().ClientToScreenScaled(r.rb, r.rb.x, r.rb.y);
    ImGui::GetWindowDrawList()->AddRect((const ImVec2&)r.lt, (const ImVec2&)r.rb, 0xFF0000FF);

    ImGui::PopClipRect();
    ImGui::End();
}

void showUiEditor(bool& show)
{
    showWndList(show);
    showWndProp(show);
    showWndHud();
}