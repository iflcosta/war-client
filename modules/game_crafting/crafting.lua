--[[
    Aethrium Aetherite & Crafting System
    Phase 5: Stable UI Fix (Global Root)
    Developer: Antigravity (Senior AI Coding Assistant)
]]--

local OPCODE_CRAFTING = 152

local craftingWindow = nil
local craftingButton = nil

function init()
    connect(g_game, { onGameStart = onGameStart,
                     onGameEnd = onGameEnd })

    -- Load UI to the Global RootWidget (Available in all OTClient versions)
    craftingWindow = g_ui.loadUI('crafting', g_ui.getRootWidget())
    if craftingWindow then
        craftingWindow:hide()
    end

    -- Register OpCode
    ProtocolGame.registerExtendedOpcode(OPCODE_CRAFTING, onExtendedOpcode)

    -- Shortcut (Ctrl + M)
    g_keyboard.bindKeyDown('Ctrl+M', toggle)

    if g_game.isOnline() then
        onGameStart()
    end
end

function terminate()
    disconnect(g_game, { onGameStart = onGameStart,
                        onGameEnd = onGameEnd })

    ProtocolGame.unregisterExtendedOpcode(OPCODE_CRAFTING)
    g_keyboard.unbindKeyDown('Ctrl+M')

    if craftingWindow then
        craftingWindow:destroy()
        craftingWindow = nil
    end

    if craftingButton then
        craftingButton:destroy()
        craftingButton = nil
    end
end

function onGameStart()
    if not craftingButton then
        -- Default top menu icon
        craftingButton = modules.client_topmenu.addLeftGameButton('craftingButton', tr('Aetherite Mastery') .. ' (Ctrl+M)', '/images/topbuttons/spelllist', toggle)
    end
    
    if craftingWindow then
        craftingWindow:hide()
    end
    
    refresh()
end

function onGameEnd()
    if craftingWindow then
        craftingWindow:hide()
    end
end

function toggle()
    if not craftingWindow then
        -- Fallback: try to load UI again if it failed
        craftingWindow = g_ui.loadUI('crafting', g_ui.getRootWidget())
        if not craftingWindow then return end
        craftingWindow:hide()
    end

    if not craftingWindow:isVisible() then
        refresh()
        craftingWindow:show()
        craftingWindow:raise()
        craftingWindow:focus()
    else
        craftingWindow:hide()
    end
end

function refresh()
    sendOpcode({action = "open_request"})
end

function sendOpcode(data)
    local protocol = g_game.getProtocolGame()
    if protocol then
        protocol:sendExtendedOpcode(OPCODE_CRAFTING, json.encode(data))
    end
end

function onExtendedOpcode(protocol, opcode, buffer)
    if opcode ~= OPCODE_CRAFTING then return end

    local status, data = pcall(json.decode, buffer)
    if not status or not data then
        return
    end

    if data.action == "open" or data.action == "update_skills" then
        updateUI(data)
    elseif data.action == "error" then
        displayError(data.message)
    end
end

function updateUI(data)
    if not craftingWindow then return end

    -- Update Virtual Dust
    if data.dust then
        local dustLabel = craftingWindow:recursiveGetChildById('dustLabel')
        if dustLabel then
            dustLabel:setText("Virtual Dust: " .. data.dust)
        end
    end

    -- Update Skills
    if data.skills then
        local skills = {"coleta", "refino", "crafting"}
        for _, skillName in ipairs(skills) do
            local skillData = data.skills[skillName]
            if skillData then
                local panel = craftingWindow:recursiveGetChildById(skillName)
                if panel then
                    local levelLabel = panel:getChildById('levelLabel')
                    if levelLabel then levelLabel:setText("Level: " .. skillData.lvl) end
                    
                    local progress = panel:getChildById('progress')
                    if progress then
                        progress:setMinimum(0)
                        progress:setMaximum(skillData.nextXp)
                        progress:setValue(skillData.xp)
                        progress:setTooltip(string.format("XP: %d / %d", skillData.xp, skillData.nextXp))
                    end

                    local priority = skillData.priority
                    for p = 1, 3 do
                        local btn = panel:getChildById('setPriority' .. p)
                        if btn then
                            if p == priority then
                                btn:setEnabled(false)
                                btn:setText("ACTIVE")
                                btn:setColor("#00ff00")
                            else
                                btn:setEnabled(true)
                                btn:setText(p .. "st Focus")
                                btn:setColor("#ffffff")
                                btn.onClick = function() 
                                    sendOpcode({action = "change_focus", skillId = skillData.id, priority = p})
                                end
                            end
                        end
                    end
                end
            end
        end
    end

    -- Update Recipes
    if data.recipes then
        updateRecipes(data.recipes)
    end
end

function updateRecipes(recipes)
    if not craftingWindow then return end
    local list = craftingWindow:recursiveGetChildById('recipeList')
    if not list then return end
    
    list:destroyChildren()

    for _, recipe in ipairs(recipes) do
        local id = recipe.id
        local widget = g_ui.createWidget('RecipeItem', list)
        widget:setId('recipe_' .. id)
        
        widget:getChildById('itemDisplay'):setItemId(recipe.resultId)
        widget:getChildById('name'):setText(recipe.name)
        
        local desc = string.format("Ouro: %dk | Slida: %d", (recipe.gold / 1000), recipe.ingredients[1].count)
        if recipe.tier == 3 then
            desc = desc .. " | + Essence"
        end
        widget:getChildById('cost'):setText(desc)
        
        widget:getChildById('craftButton').onClick = function()
            sendOpcode({action = "craft", recipeId = id})
        end
    end
end

function displayError(message)
    if not craftingWindow then return end
    local msgBox = displayErrorBox(tr('Erro de Forja'), message)
    if msgBox then
        msgBox:raise()
        msgBox:focus()
    end
end
