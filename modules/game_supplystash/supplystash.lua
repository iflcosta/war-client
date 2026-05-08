local window
withdrawWindow = nil

local function registerProtocol()
	ProtocolGame.registerOpcode(0x29,
        function(protocol, msg)
			local itemData = {}
            for i = 1, msg:getU16() do
				table.insert(itemData, {msg:getU16(), msg:getU32()})
            end
		
			setup(itemData, msg:getU16())
        end
    )
end

function init()	
	
	-- Main stash window
	window 	   = g_ui.displayUI('supplystash')
	freeSlots = window:recursiveGetChildById('freeSlots')
	
	-- Selecter for charms
	itemsContainer = window:recursiveGetChildById('itemsContainer')
	supplyItems = itemsContainer:recursiveGetChildById('supplyItems')
	
	connect(
        g_game,
        {
            onEnterGame = registerProtocol,
            onPendingGame = registerProtocol
        }
    )
	
	createwithdrawWindow()
	
    if g_game.isOnline() then
        registerProtocol()
    end
end

function terminate()
	disconnect(
        g_game,
        {
            onEnterGame = registerProtocol,
            onPendingGame = registerProtocol
        }
    )

    ProtocolGame.unregisterOpcode(0x29)
	window:destroy()
	withdrawWindow:destroy()
end

function toggle()
	if window:isVisible() then
		window:hide()
		window:unlock()
		modules.game_interface.getRootPanel():focus()
	else
		window:show()
		window:raise()
		window:focus()
		modules.game_interface.getRootPanel():focus()
		window:lock()
	end
end

function createwithdrawWindow()
	if withdrawWindow then return end
	withdrawWindow = g_ui.displayUI('withdraw')
	withdrawWindow:hide()
end

function withdrawHide()
	withdrawWindow:hide()
end	

function placeholder()

end

function emptyItemList()
	while supplyItems:getChildCount() > 0 do
		local child = supplyItems:getLastChild()
		supplyItems:destroyChildren(child)
	end
end

function setup(itemData, sizeLeft)
	toggle() -- We show the window
	emptyItemList() -- We empty the item list
	
	if (#itemData >= 1) then
		for i = 1, #itemData do
			local row = g_ui.createWidget('StashItem', supplyItems)
			row.index = i
			row:setId("stashItem"..i)
			row.categoryId = i
			row:setItemId(itemData[i][1])
			
			local countText = row:recursiveGetChildById('count')
			countText:setText(itemData[i][2])
			
			row.onClick = function(self)
				window:hide()
				window:unlock()
				modules.game_interface.getRootPanel():focus()
				withdrawWindow:show()
				withdrawWindow:raise()
				withdrawWindow:focus()
				withdrawWindow:unlock()
				modules.game_interface.getRootPanel():focus()
				
				withdrawWindow.item:setItemId(itemData[i][1])
				withdrawWindow.count:setText(1)
				withdrawWindow.countScrollBar:setMinimum(1)
				withdrawWindow.countScrollBar:setMaximum(itemData[i][2])
				withdrawWindow.countScrollBar:setValue(1)
				withdrawWindow.countScrollBar.onValueChange = function(widget, value)
					withdrawWindow.count:setText(value)
				end
				
				local buttonCancel = withdrawWindow:recursiveGetChildById('buttonCancel')
				buttonCancel.onClick = function(self)
					withdrawHide()
					window:show()
					window:raise()
					window:focus()
					window:lock()
					modules.game_interface.getRootPanel():focus()
				end
				
				local okButton = withdrawWindow:recursiveGetChildById('buttonOk')
				okButton.onClick = function(self)
					local count = withdrawWindow:recursiveGetChildById('count'):getText()
					
						local protocolGame = g_game.getProtocolGame()
	if protocolGame then
    local msg = OutputMessage.create()
    msg:addU8(0x28)
    msg:addU8(3)
    msg:addU16(itemData[i][1])
    msg:addU32(tonumber(count))
    msg:addU8(0)
    protocolGame:send(msg)
	end  

					withdrawHide()
					modules.game_interface.getRootPanel():focus()
				end
			end
		end
	end
	
	freeSlots:setText("Free slots: "..sizeLeft)
end