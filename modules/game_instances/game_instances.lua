-- ============================================================
-- Aethrium — Hunt & Boss Room Instance Selector
-- Opcode 151 | Dark Theme
-- ============================================================

local INSTANCES_OPCODE = 151

local instancesWindow  = nil
local instancesButton  = nil
local currentTab       = "hunts"   -- "hunts" | "bosses"
local currentFilter    = "all"     -- "all" | "coins" | "tasks"
local cachedHunts      = {}
local cachedBosses     = {}

-- ── Lifecycle ────────────────────────────────────────────────

function init()
    g_ui.importStyle("game_instances.otui")

    ProtocolGame.registerExtendedOpcode(INSTANCES_OPCODE, onExtendedOpcode)

    instancesWindow = g_ui.createWidget("InstancesWindow", g_ui.getRootWidget())
    if instancesWindow then
        instancesWindow:hide()
    end

    instancesButton = modules.game_mainpanel.addToggleButton(
        "instancesButton",
        tr("Hunts & Boss Rooms"),
        "/modules/game_cyclopedia/images/boss/icon_star_gold.png",
        toggle
    )
end

function terminate()
    ProtocolGame.unregisterExtendedOpcode(INSTANCES_OPCODE)

    if instancesWindow then
        instancesWindow:destroy()
        instancesWindow = nil
    end

    if instancesButton then
        instancesButton:destroy()
        instancesButton = nil
    end

    cachedHunts  = {}
    cachedBosses = {}
end

-- ── Network ──────────────────────────────────────────────────

local function ensureWindow()
    if not instancesWindow then
        instancesWindow = g_ui.createWidget("InstancesWindow", g_ui.getRootWidget())
        if instancesWindow then
            instancesWindow:hide()
        end
    end
    return instancesWindow
end

function toggle()
    if not g_game.isOnline() then return end

    local win = ensureWindow()
    if not win then
        g_logger.error("[Instances] InstancesWindow widget nao encontrado — verifique game_instances.otui")
        return
    end

    if win:isVisible() then
        win:hide()
    else
        -- Pede dados frescos ao servidor
        g_game.getProtocolGame():sendExtendedOpcode(INSTANCES_OPCODE,
            json.encode({ action = "open_request" }))
    end
end

function onExtendedOpcode(protocol, opcode, buffer)
    local ok, data = pcall(json.decode, buffer)
    if not ok or type(data) ~= "table" then
        g_logger.error("[Instances] Bad JSON: " .. tostring(buffer))
        return
    end

    if data.action == "open" then
        cachedHunts  = data.hunts  or {}
        cachedBosses = data.bosses or {}

        local win = ensureWindow()
        if not win then return end

        -- Atualiza saldos do jogador
        local resourcesLabel = win:getChildById("filterBar"):getChildById("playerResourcesLabel")
        if resourcesLabel then
            resourcesLabel:setText(string.format(
                "Task Pts: %d  |  Coins: %d",
                data.taskPoints or 0,
                data.coins or 0
            ))
        end

        refreshAll()

        if not win:isVisible() then
            win:show()
            win:raise()
            win:focus()
        end

    elseif data.action == "error" then
        displayErrorBox(tr("Instância"), data.message or "Erro desconhecido.")
    end
end

-- ── Tab & Filter ─────────────────────────────────────────────

function switchTab(name)
    currentTab = name
    local win = instancesWindow

    local tabBar    = win:getChildById("tabBar")
    local tabHunts  = tabBar:getChildById("tabHunts")
    local tabBosses = tabBar:getChildById("tabBosses")
    local huntsPanel  = win:getChildById("huntsPanel")
    local bossesPanel = win:getChildById("bossesPanel")

    local function setActive(btn)
        btn:setColor("#00E5FF")
        btn:setBorderColor("#00E5FF")
        btn:setBackgroundColor("#1A1A2E")
    end
    local function setInactive(btn)
        btn:setColor("#888888")
        btn:setBorderColor("#333333")
        btn:setBackgroundColor("#0D0D14")
    end

    if name == "hunts" then
        huntsPanel:show()
        bossesPanel:hide()
        setActive(tabHunts)
        setInactive(tabBosses)
    else
        bossesPanel:show()
        huntsPanel:hide()
        setActive(tabBosses)
        setInactive(tabHunts)
    end
end

function setFilter(filter)
    currentFilter = filter
    local win = instancesWindow
    local filterBar = win:getChildById("filterBar")
    local all    = filterBar:getChildById("filterAll")
    local coins  = filterBar:getChildById("filterCoins")
    local tasks  = filterBar:getChildById("filterTasks")

    -- Reseta cores
    for _, btn in ipairs({all, coins, tasks}) do
        btn:setColor("#AAAAAA")
        btn:setBorderColor("#333333")
        btn:setBackgroundColor("#0D0D14")
    end

    -- Destaca o ativo
    local active = filter == "all" and all or (filter == "coins" and coins or tasks)
    active:setColor("#00E5FF")
    active:setBorderColor("#00E5FF")
    active:setBackgroundColor("#1A1A2E")

    populateHunts()
    populateBosses()
end

-- ── Render ───────────────────────────────────────────────────

function refreshAll()
    populateHunts()
    populateBosses()
    switchTab(currentTab)
    -- aplica highlight do filtro ativo sem re-popular
    local win = instancesWindow
    local filterBar = win:getChildById("filterBar")
    local all    = filterBar:getChildById("filterAll")
    local coins  = filterBar:getChildById("filterCoins")
    local tasks  = filterBar:getChildById("filterTasks")
    for _, btn in ipairs({all, coins, tasks}) do
        btn:setColor("#AAAAAA")
        btn:setBorderColor("#333333")
        btn:setBackgroundColor("#0D0D14")
    end
    local active = currentFilter == "all" and all or (currentFilter == "coins" and coins or tasks)
    active:setColor("#00E5FF")
    active:setBorderColor("#00E5FF")
    active:setBackgroundColor("#1A1A2E")
end

local function formatCooldown(seconds)
    if seconds <= 0 then return nil end
    local h = math.floor(seconds / 3600)
    local m = math.floor((seconds % 3600) / 60)
    local s = seconds % 60
    if h > 0 then
        return string.format("CD: %dh %dm", h, m)
    elseif m > 0 then
        return string.format("CD: %dm %ds", m, s)
    else
        return string.format("CD: %ds", s)
    end
end

local function formatPrice(entry)
    if entry.currency == "coins" then
        return string.format("%d Tibia Coins", entry.price)
    else
        return string.format("%d Task Points", entry.price)
    end
end

local function makeCard(parent, entry, enterAction, infoText)
    local card = g_ui.createWidget("InstanceCard", parent)

    card:getChildById("cardTitle"):setText(entry.name)
    card:getChildById("cardLevel"):setText(string.format("Nivel: %d+", entry.level))

    local creatureWidget = card:getChildById("cardCreature")
    if entry.looktype and entry.looktype > 0 then
        local outfit = { type = entry.looktype, head = 0, body = 0, legs = 0, feet = 0, addons = 0 }
        creatureWidget:setOutfit(outfit)
        creatureWidget:show()
    else
        creatureWidget:hide()
        -- sem creature: labels ocupam a largura toda
        local labels = {"cardLevel", "cardPrice", "cardInfo"}
        for _, id in ipairs(labels) do
            local lbl = card:getChildById(id)
            lbl:setMarginLeft(0)
            lbl:fill("parent.left", true)
        end
    end

    local priceLabel = card:getChildById("cardPrice")
    priceLabel:setText(formatPrice(entry))
    if entry.currency == "coins" then
        priceLabel:setColor("#00E5FF")
    else
        priceLabel:setColor("#FFD700")
    end

    card:getChildById("cardInfo"):setText(infoText)

    local cdLabel = card:getChildById("cardCooldown")
    local cdText  = formatCooldown(entry.cooldown or 0)
    if cdText then
        cdLabel:setText(cdText)
        cdLabel:show()
    else
        cdLabel:hide()
    end

    local btn = card:getChildById("cardEnterBtn")
    if cdText then
        btn:setText(tr("Em Cooldown"))
        btn:setEnabled(false)
    else
        btn:setText(tr("Entrar"))
        btn:setEnabled(true)
        btn.onClick = enterAction
    end

    return card
end

function populateHunts()
    local list = instancesWindow:getChildById("huntsPanel"):getChildById("huntsList")
    list:destroyChildren()

    for _, hunt in ipairs(cachedHunts) do
        if currentFilter == "all" or currentFilter == hunt.currency then
            local monsters = table.concat(hunt.monsters or {}, ", ")
            local infoText = string.format("%.30s", monsters)
            if #monsters > 30 then infoText = infoText .. "…" end

            makeCard(list, hunt, function()
                confirmEnter("hunt", hunt)
            end, infoText)
        end
    end
end

function populateBosses()
    local list = instancesWindow:getChildById("bossesPanel"):getChildById("bossesList")
    list:destroyChildren()

    for _, boss in ipairs(cachedBosses) do
        if currentFilter == "all" or currentFilter == boss.currency then
            local infoText = string.format("%s | %dm | %d-%d players",
                boss.bossName or boss.name,
                boss.killTime or 0,
                boss.minPlayers or 1,
                boss.maxPlayers or 1)

            makeCard(list, boss, function()
                confirmEnter("boss", boss)
            end, infoText)
        end
    end
end

-- ── Confirmação de entrada ────────────────────────────────────

function confirmEnter(kind, entry)
    local title = kind == "hunt" and tr("Entrar na Hunt") or tr("Entrar na Boss Room")
    local msg   = string.format(
        "%s\n\nCusto: %s\nLevel mínimo: %d\n\nDeseja entrar?",
        entry.name,
        formatPrice(entry),
        entry.level
    )

    local box
    local function onConfirm()
        box:ok()
        local action = kind == "hunt" and "enter_hunt" or "enter_boss"
        g_game.getProtocolGame():sendExtendedOpcode(INSTANCES_OPCODE,
            json.encode({ action = action, id = entry.id }))
        instancesWindow:hide()
    end
    local function onCancel()
        box:cancel()
    end

    box = displayGeneralBox(title, msg,
        { { text = tr("Confirmar"), callback = onConfirm },
          { text = tr("Cancelar"),  callback = onCancel  } },
        onConfirm, onCancel)
end

-- ── Sair da instância ─────────────────────────────────────────

function leaveInstance()
    local box
    local function onConfirm()
        box:ok()
        g_game.getProtocolGame():sendExtendedOpcode(INSTANCES_OPCODE,
            json.encode({ action = "leave" }))
    end
    local function onCancel()
        box:cancel()
    end

    box = displayGeneralBox(
        tr("Sair da Instância"),
        tr("Tem certeza que deseja sair? Você será teleportado ao templo."),
        { { text = tr("Sair"),     callback = onConfirm },
          { text = tr("Cancelar"), callback = onCancel  } },
        onConfirm, onCancel)
end
