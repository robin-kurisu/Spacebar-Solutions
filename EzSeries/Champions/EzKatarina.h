#pragma once
#include "EzChampion.h"


class EzKatarina : public EzChampion {
    public:
        static auto on_boot(IMenu * menu) -> IMenu*;
        static auto on_issue_order(IGameObject * unit, OnIssueOrderEventArgs * args) -> void;
        static auto on_update() -> void;
        static auto on_pre_update() -> void;
        static auto on_post_update() -> void;
        static auto on_hud_draw() -> void;
        static auto hpbarfill_render() -> void;

        static std::map<float, Vector> blade_vectors;
        static std::map<float, IGameObject *> all_blades;

        static auto shunpo_position(IGameObject * unitNear)->Vector;
        static auto dynamic_range() -> float;
        static auto on_buff(IGameObject * unit, OnBuffEventArgs * args);
        static auto on_create(IGameObject * unit) -> void;
        static auto on_destroy(IGameObject * unit) -> void;
        static auto on_do_cast(IGameObject * unit, OnProcessSpellEventArgs * args) -> void;
        static auto on_spell_cast(IGameObject * unit, OnProcessSpellEventArgs * args) -> void; };

std::map<float, Vector> EzKatarina::blade_vectors;
std::map<float, IGameObject *> EzKatarina::all_blades;

inline auto EzKatarina::on_boot(IMenu * menu) -> IMenu * {
    auto d = menu->AddSubMenu("Katarina: Draw", "katarina.xdraw");
    Menu["katarina.xdraw.q"] = d->AddCheckBox("Draw Q Range", "katarina.xdraw.q", true);
    Menu["katarina.xdraw.q1"] = d->AddColorPicker("Q Range Color", "katarina.xdraw.q1", 204, 102, 102, 115);
    Menu["katarina.xdraw.e"] = d->AddCheckBox("Draw E Range", "katarina.xdraw.w2", true);
    Menu["katarina.xdraw.e1"] = d->AddColorPicker("E Range Color", "katarina.xdraw.e1", 204, 102, 102, 115);
    Menu["katarina.xdraw.r"] = d->AddCheckBox("Draw R Range", "katarina.xdraw.r", true);
    Menu["katarina.xdraw.r1"] = d->AddColorPicker("R Range Color", "katarina.xdraw.r1", 204, 102, 102, 115);
    Menu["katarina.xdraw.dagger"] = d->AddCheckBox("Debug Shunpo Position", "katarina.xdraw.dagger", false);
    Menu["katarina.xdraw.dagger1"] = d->AddColorPicker("Position Color", "katarina.xdraw.dagger1", 204, 102, 102, 115);
    Menu["katarina.use.q"] = menu->AddCheckBox("Use Bouncing Blades", "katarina.use.q", true);
    Menu["katarina.use.w"] = menu->AddCheckBox("Use Preparation", "katarina.use.w", true);
    Menu["katarina.use.e"] = menu->AddCheckBox("Use Shunpo", "katarina.use.e", true);
    Menu["katarina.use.r"] = menu->AddCheckBox("Use Death Lotus", "katarina.use.r", true);
    auto m = menu->AddSubMenu("Katarina: Mechanics", "katarina.mechanics");
    auto death_lotus_menu = m->AddSubMenu("Death Lotus Settings", "death.lotus.menu");
    Spells["katarina.q"] = g_Common->AddSpell(SpellSlot::Q, 625);
    Spells["katarina.w"] = g_Common->AddSpell(SpellSlot::W, 340);
    Spells["katarina.e"] = g_Common->AddSpell(SpellSlot::E, 725);
    Spells["katarina.r"] = g_Common->AddSpell(SpellSlot::R, 550);
    g_Common->Log("[EzSeries]: Katarina Loaded!");
    g_Common->ChatPrint(R"(<font color="#CC6666"><b>[EzSeries Katarina]:</b></font><b><font color="#99FF99"> Loaded!</font>)");
    return menu; }

inline auto EzKatarina::on_issue_order(IGameObject * unit, OnIssueOrderEventArgs * args) -> void {
    if(unit->IsMe() && unit->HasBuff("katarinarsound")) {
        if(unit->HasBuff("katarinarsound") &&
            unit->CountEnemiesInRange(Spells["katarina.r"]->Range()) > 0) {
            args->Process = false; } } }

inline auto EzKatarina::on_pre_update() -> void {}

inline auto EzKatarina::on_post_update() -> void {
    // - dagger handling
    for(auto i : all_blades) {
        auto key = i.first;
        auto blade = i.second;

        // - remove invalid? blades
        if(blade == nullptr || !blade->IsValid() || !blade->IsVisible()) {
            all_blades.erase(key);
            g_Common->Log("bladed deleted: invalid");
            break; }

        // - remove blades ive picked up
        if(blade->Distance(g_LocalPlayer) <= Spells["katarina.w"]->Range()) {
            g_Common->Log("bladed deleted: proximity");
            all_blades.erase(key);
            break; }

        // - remove blade after 4 seconds
        if(g_Common->Time() - key > 4) {
            g_Common->Log("bladed deleted: expiry");
            all_blades.erase(key);
            break; } } }

inline auto EzKatarina::on_update() -> void {
    on_pre_update();

    // todo: full combo, combo logic tweaks+
    if(g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeCombo)) {
        // - don't interrupt ult like dis
        if(g_LocalPlayer->HasBuff("katarinarsound") &&
            g_LocalPlayer->CountEnemiesInRange(Spells["katarina.r"]->Range()) > 0) {
            return; }

        // - shunpo target
        if(Spells["katarina.e"]->IsReady() && Menu["katarina.use.e"]->GetBool()) {
            auto target = g_Common->GetTarget(Spells["katarina.e"]->Range(), DamageType::Magical);
            const auto position = shunpo_position(target);

            if(position.IsValid() &&
                g_LocalPlayer->Distance(position) > Spells["katarina.w"]->Range()) {
                if(Spells["katarina.e"]->FastCast(position)) {
                    g_Orbwalker->ResetAA(); } } }

        // - preparation
        if(Spells["katarina.w"]->IsReady() && Menu["katarina.use.w"]->GetBool()) {
            auto target = g_Common->GetTarget(Spells["katarina.w"]->Range(), DamageType::Magical);

            if(target != nullptr && target->IsValidTarget()) {
                Spells["katarina.w"]->Cast(); } }

        // - death lotus
        if(Spells["katarina.r"]->IsReady() && Menu["katarina.use.r"]->GetBool()) {
            auto target = g_Common->GetTarget(dynamic_range() + 100, DamageType::Magical);

            // -- temp logic for now
            if(!Spells["katarina.q"]->IsReady() && !Spells["katarina.w"]->IsReady() && !Spells["katarina.e"]->IsReady()) {
                if(target != nullptr && target->IsValidTarget()) {
                    Spells["katarina.r"]->Cast(); } } }

        // - bouncing blades
        if(Spells["katarina.q"]->IsReady() && Menu["katarina.use.q"]->GetBool()) {
            auto target = g_Common->GetTarget(dynamic_range(), DamageType::Magical);

            if(target != nullptr && target->IsValidTarget()) {
                if(!target->IsInAutoAttackRange(g_LocalPlayer)) {
                    Spells["katarina.q"]->Cast(target); } } } }

    if(g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeLaneClear) || g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeMixed)) {
        // - don't interrupt ult like dis
        // - for now
        if(g_LocalPlayer->HasBuff("katarinarsound") &&
            g_LocalPlayer->CountEnemiesInRange(Spells["katarina.r"]->Range()) > 0) {
            return; }

        // - preparation
        if(Spells["katarina.w"]->IsReady() && Menu["katarina.use.w"]->GetBool()) {
            auto target = g_Common->GetTarget(Spells["katarina.w"]->Range(), DamageType::Magical);

            if(target != nullptr && target->IsValidTarget()) {
                Spells["katarina.w"]->Cast(); } }

        // - bouncing blades
        if(Spells["katarina.q"]->IsReady() && Menu["katarina.use.q"]->GetBool()) {
            auto target = g_Common->GetTarget(Spells["katarina.q"]->Range(), DamageType::Magical);

            if(target != nullptr && target->IsValidTarget()) {
                if(!target->IsInAutoAttackRange(g_LocalPlayer)) {
                    Spells["katarina.q"]->Cast(target); } } } }

    on_post_update(); }


inline auto EzKatarina::on_hud_draw() -> void {
    // - debug
    auto t = g_Common->GetTarget(Spells["katarina.e"]->Range(), DamageType::Magical);

    if(t != nullptr && t->IsValidTarget() && Menu["katarina.xdraw.dagger"]->GetBool()) {
        g_Drawing->AddCircle(shunpo_position(t), 50, Menu["katarina.xdraw.dagger1"]->GetColor(), 4); }

    if(Menu["katarina.xdraw.q"]->GetBool()) {
        g_Drawing->AddCircle(g_LocalPlayer->Position(), Spells["katarina.q"]->Range(), Menu["katarina.xdraw.q1"]->GetColor(), 2); }

    if(Menu["katarina.xdraw.e"]->GetBool()) {
        g_Drawing->AddCircle(g_LocalPlayer->Position(), Spells["katarina.e"]->Range(), Menu["katarina.xdraw.e1"]->GetColor(), 2); }

    if(Menu["katarina.xdraw.r"]->GetBool()) {
        g_Drawing->AddCircle(g_LocalPlayer->Position(), Spells["katarina.r"]->Range(), Menu["katarina.xdraw.r1"]->GetColor(), 2); } }

inline auto EzKatarina::hpbarfill_render() -> void {}

inline auto EzKatarina::shunpo_position(IGameObject * unitNear) -> Vector {
    if(unitNear == nullptr || !unitNear->IsValidTarget()) {
        return { 0, 0, 0 }; }

    for(const auto b : all_blades) {
        auto key = b.first;
        auto blade = b.second;

        if(unitNear->Distance(blade) <= Spells["katarina.w"]->Range() + unitNear->BoundingRadius()
            && blade->Distance(g_LocalPlayer) <= Spells["katarina.e"]->Range() + Spells["katarina.w"]->Range()) {
            if(blade->Distance(g_LocalPlayer) > Spells["katarina.w"]->Range() - 75) {
                return blade->ServerPosition(); } }  }

    // - front of unit (for now)
    return unitNear->ServerPosition() + (g_LocalPlayer->ServerPosition() - unitNear->ServerPosition()).Normalized() * 135; }

inline auto EzKatarina::dynamic_range() -> float {
    if(!Spells["katarina.e"]->IsReady()) {
        if(g_Common->TickCount() - Spells["katarina.e"]->LastCastTime() < 500) {
            return Spells["katarina.w"]->Range(); } }

    return Spells["katarina.q"]->Range(); }

inline auto EzKatarina::on_buff(IGameObject * unit, OnBuffEventArgs * args) {}

inline auto EzKatarina::on_create(IGameObject * unit) -> void {
    if(StringContains(unit->Name().c_str(), "Dagger_Land")) {
        if(unit != nullptr && unit->IsValid() && unit->IsVisible()) {
            all_blades[g_Common->Time()] = unit; } } }

inline auto EzKatarina::on_destroy(IGameObject * unit) -> void {}

inline auto EzKatarina::on_do_cast(IGameObject * unit, OnProcessSpellEventArgs * args) -> void {
    if(!unit->IsMe() || !args->IsAutoAttack) {
        return; }

    if(g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeCombo)) {
        // - aa -> shunpo reset
        if(Spells["katarina.e"]->IsReady() && Menu["katarina.use.e"]->GetBool()) {
            if(args->Target != nullptr && args->Target->IsValidTarget() && args->Target->IsAIHero()) {
                const auto target = g_Common->GetTarget(Spells["katarina.e"]->Range(), DamageType::Magical);
                const auto position = shunpo_position(target);

                if(position.IsValid() &&
                    g_LocalPlayer->Distance(position) > Spells["katarina.w"]->Range()) {
                    if(Spells["katarina.e"]->FastCast(position)) {
                        g_Orbwalker->ResetAA(); } } } }

        // - bouncing blades
        if(Spells["katarina.q"]->IsReady() && Menu["katarina.use.q"]->GetBool()) {
            if(args->Target != nullptr && args->Target->IsValidTarget() && args->Target->IsAIHero()) {
                Spells["katarina.q"]->Cast(args->Target); } } }

    if(g_Orbwalker->IsModeActive(eOrbwalkingMode::kModeMixed)) {
        // - bouncing blades
        if(Spells["katarina.q"]->IsReady() && Menu["katarina.use.q"]->GetBool()) {
            if(args->Target != nullptr && args->Target->IsValidTarget() && args->Target->IsAIHero()) {
                Spells["katarina.q"]->Cast(args->Target); } } } }

inline auto EzKatarina::on_spell_cast(IGameObject * unit, OnProcessSpellEventArgs * args) -> void {}