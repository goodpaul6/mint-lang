# sfml.mt -- binding of csfml to mint

extern type

extern sfTransform_create
extern sfTransform_identity
extern sfTransform_transformPoint
extern sfTransform_transformRect
extern sfTransform_combine
extern sfTransform_translate
extern sfTransform_rotate
extern sfTransform_rotateWithCenter
extern sfTransform_scale
extern sfTransform_scaleWithCenter

extern sfBlendMode_getBlendAlpha
extern sfBlendMode_getBlendAdd
extern sfBlendMode_getBlendMultiply
extern sfBlendMode_getBlendNone

extern sfVertex_create
extern sfVertex_setPosition
extern sfVertex_setColor
extern sfVertex_setTexCoords

extern sfVertexArray_create
extern sfVertexArray_copy
extern sfVertexArray_getVertexCount
extern sfVertexArray_clear
extern sfVertexArray_resize
extern sfVertexArray_append
extern sfVertexArray_setPrimitiveType

extern sfRenderStates_create
extern sfRenderStates_setBlendMode
extern sfRenderStates_setTransform
extern sfRenderStates_setTexture
extern sfRenderStates_setShader

extern sfRenderWindow_create
extern sfRenderWindow_setFramerateLimit
extern sfRenderWindow_isOpen
extern sfRenderWindow_close
extern sfRenderWindow_pollEvent
extern sfRenderWindow_clear
extern sfRenderWindow_display
extern sfRenderWindow_drawSprite
extern sfRenderWindow_drawVertexArray

extern sfEvent_create
extern sfEvent_type
extern sfEvent_key_code

extern sfTexture_createFromFile
extern sfTexture_getSize

var sfEvtClosed
var sfEvtResized
var sfEvtLostFocus
var sfEvtGainedFocus
var sfEvtTextEntered
var sfEvtKeyPressed
var sfEvtKeyReleased
var sfEvtMouseWheelMoved
var sfEvtMouseButtonPressed
var sfEvtMouseButtonReleased
var sfEvtMouseMoved
var sfEvtMouseEntered
var sfEvtMouseLeft
var sfEvtJoystickButtonPressed
var sfEvtJoystickButtonReleased
var sfEvtJoystickMoved
var sfEvtJoystickConnected
var sfEvtJoystickDisconnected
var sfEvtTouchBegan
var sfEvtTouchMoved
var sfEvtTouchEnded
var sfEvtSensorChanged
var sfPi

var sfKeyA
var sfKeyB
var sfKeyC
var sfKeyD
var sfKeyE
var sfKeyF
var sfKeyG
var sfKeyH
var sfKeyI
var sfKeyJ
var sfKeyK
var sfKeyL
var sfKeyM
var sfKeyN
var sfKeyO
var sfKeyP
var sfKeyQ
var sfKeyR
var sfKeyS
var sfKeyT
var sfKeyU
var sfKeyV
var sfKeyW
var sfKeyX
var sfKeyY
var sfKeyZ
var sfKeyNum0
var sfKeyNum1
var sfKeyNum2
var sfKeyNum3
var sfKeyNum4
var sfKeyNum5
var sfKeyNum6
var sfKeyNum7
var sfKeyNum8
var sfKeyNum9
var sfKeyEscape
var sfKeyLControl
var sfKeyLShift
var sfKeyLAlt
var sfKeyLSystem
var sfKeyRControl
var sfKeyRShift
var sfKeyRAlt
var sfKeyRSystem
var sfKeyMenu
var sfKeyLBracket
var sfKeyRBracket
var sfKeySemiColon
var sfKeyComma
var sfKeyPeriod
var sfKeyQuote
var sfKeySlash
var sfKeyBackSlash
var sfKeyTilde
var sfKeyEqual
var sfKeyDash
var sfKeySpace
var sfKeyReturn
var sfKeyBackSpace
var sfKeyTab
var sfKeyPageUp
var sfKeyPageDown
var sfKeyEnd
var sfKeyHome
var sfKeyInsert
var sfKeyDelete
var sfKeyAdd
var sfKeySubtract
var sfKeyMultiply
var sfKeyDivide
var sfKeyLeft
var sfKeyRight
var sfKeyUp
var sfKeyDown
var sfKeyNumpad0
var sfKeyNumpad1
var sfKeyNumpad2
var sfKeyNumpad3
var sfKeyNumpad4
var sfKeyNumpad5
var sfKeyNumpad6
var sfKeyNumpad7
var sfKeyNumpad8
var sfKeyNumpad9
var sfKeyF1
var sfKeyF2
var sfKeyF3
var sfKeyF4
var sfKeyF5
var sfKeyF6
var sfKeyF7
var sfKeyF8
var sfKeyF9
var sfKeyF10
var sfKeyF11
var sfKeyF12
var sfKeyF13
var sfKeyF14
var sfKeyF15
var sfKeyPause
var sfNumKeys

var sfBlendAlpha
var sfBlendAdd
var sfBlendMultiply
var sfBlendNone

var sfPoints
var sfLines
var sfLinesStrip
var sfTriangles
var sfTrianglesStrip
var sfTrianglesFan
var sfQuads

func sfInit()
	sfEvtClosed = 0
	sfEvtResized = 1
	sfEvtLostFocus = 2
	sfEvtGainedFocus = 3
	sfEvtTextEntered = 4
	sfEvtKeyPressed = 5
	sfEvtKeyReleased = 6
	sfEvtMouseWheelMoved = 7
	sfEvtMouseButtonPressed = 8
	sfEvtMouseButtonReleased = 9
	sfEvtMouseMoved = 10
	sfEvtMouseEntered = 11
	sfEvtMouseLeft = 12
	sfEvtJoystickButtonPressed = 13
	sfEvtJoystickButtonReleased = 14
	sfEvtJoystickMoved = 15
	sfEvtJoystickConnected = 16
	sfEvtJoystickDisconnected = 17
	sfEvtTouchBegan = 18
	sfEvtTouchMoved = 19
	sfEvtTouchEnded = 20
	sfEvtSensorChanged = 21
	sfPi = 3.141592653
	
	sfKeyA = 0
	sfKeyB = 1
	sfKeyC = 2
	sfKeyD = 3
	sfKeyE = 4
	sfKeyF = 5
	sfKeyG = 6
	sfKeyH = 7
	sfKeyI = 8
	sfKeyJ = 9
	sfKeyK = 10
	sfKeyL = 11
	sfKeyM = 12
	sfKeyN = 13
	sfKeyO = 14
	sfKeyP = 15
	sfKeyQ = 16
	sfKeyR = 17
	sfKeyS = 18
	sfKeyT = 19
	sfKeyU = 20
	sfKeyV = 21
	sfKeyW = 22
	sfKeyX = 23
	sfKeyY = 24
	sfKeyZ = 25
	sfKeyNum0 = 26
	sfKeyNum1 = 27
	sfKeyNum2 = 28
	sfKeyNum3 = 29
	sfKeyNum4 = 30
	sfKeyNum5 = 31
	sfKeyNum6 = 32
	sfKeyNum7 = 33
	sfKeyNum8 = 34
	sfKeyNum9 = 35
	sfKeyEscape = 36
	sfKeyLControl = 37
	sfKeyLShift = 38
	sfKeyLAlt = 39
	sfKeyLSystem = 40
	sfKeyRControl = 41
	sfKeyRShift = 42
	sfKeyRAlt = 43
	sfKeyRSystem = 44
	sfKeyMenu = 45
	sfKeyLBracket = 46
	sfKeyRBracket = 47
	sfKeySemiColon = 48
	sfKeyComma = 49
	sfKeyPeriod = 50
	sfKeyQuote = 51
	sfKeySlash = 52
	sfKeyBackSlash = 53
	sfKeyTilde = 54
	sfKeyEqual = 55
	sfKeyDash = 56
	sfKeySpace = 57
	sfKeyReturn = 58
	sfKeyBackSpace = 59
	sfKeyTab = 60
	sfKeyPageUp = 61
	sfKeyPageDown = 62
	sfKeyEnd = 63
	sfKeyHome = 64
	sfKeyInsert = 65
	sfKeyDelete = 66
	sfKeyAdd = 67
	sfKeySubtract = 68
	sfKeyMultiply = 69
	sfKeyDivide = 70
	sfKeyLeft = 71
	sfKeyRight = 72
	sfKeyUp = 73
	sfKeyDown = 74
	sfKeyNumpad0 = 75
	sfKeyNumpad1 = 76
	sfKeyNumpad2 = 77
	sfKeyNumpad3 = 78
	sfKeyNumpad4 = 79
	sfKeyNumpad5 = 80
	sfKeyNumpad6 = 81
	sfKeyNumpad7 = 82
	sfKeyNumpad8 = 83
	sfKeyNumpad9 = 84
	sfKeyF1 = 85
	sfKeyF2 = 86
	sfKeyF3 = 87
	sfKeyF4 = 88
	sfKeyF5 = 89
	sfKeyF6 = 90
	sfKeyF7 = 91
	sfKeyF8 = 92
	sfKeyF9 = 93
	sfKeyF10 = 94
	sfKeyF11 = 95
	sfKeyF12 = 96
	sfKeyF13 = 97
	sfKeyF14 = 98
	sfKeyF15 = 99
	sfKeyPause = 100
	
	sfNumKeys = 256
	
	sfBlendAlpha = sfBlendMode_getBlendAlpha()
	sfBlendAdd = sfBlendMode_getBlendAdd()
	sfBlendMultiply = sfBlendMode_getBlendMultiply()
	sfBlendNone = sfBlendMode_getBlendNone()
	
	sfPoints = 0
	sfLines = 1
	sfLinesStrip = 2
	sfTriangles = 3
	sfTrianglesStrip = 4
	sfTrianglesFan = 5
	sfQuads = 6
end

func sfRenderWindow_draw(window, drawable, states)
	drawable:draw(window, states)
end

func sfRenderStates_createDefault()	
	var states = sfRenderStates_create()
	
	sfRenderStates_setBlendMode(states, sfBlendAlpha)
	sfRenderStates_setTexture(states, null)
	sfRenderStates_setTransform(states, sfTransform_create())
	sfRenderStates_setShader(states, null)
	
	return states
end

func sfVertexArray_appendQuad(vertexArray, width, height, color)
	sfVertexArray_setPrimitiveType(vertexArray, sfQuads)
	
	var vertex = sfVertex_create()
	sfVertex_setColor(vertex, color)
	
	sfVertex_setPosition(vertex, [0, 0])
	sfVertex_setTexCoords(vertex, [0, 0])
	
	sfVertexArray_append(vertexArray, vertex)
	
	sfVertex_setPosition(vertex, [width, 0])
	sfVertex_setTexCoords(vertex, [width, 0])
	
	sfVertexArray_append(vertexArray, vertex)
	
	sfVertex_setPosition(vertex, [width, height])
	sfVertex_setTexCoords(vertex, [width, height])
	
	sfVertexArray_append(vertexArray, vertex)
	
	sfVertex_setPosition(vertex, [0, height])
	sfVertex_setTexCoords(vertex, [0, height])
	
	sfVertexArray_append(vertexArray, vertex)
end

func sfVertexArray_appendQuadEx(vertexArray, x, y, width, height, color, u1, u2, u3, u4)
	sfVertexArray_setPrimitiveType(vertexArray, sfQuads)
	
	var vertex = sfVertex_create()
	sfVertex_setColor(vertex, color)
	
	sfVertex_setPosition(vertex, [x, y])
	sfVertex_setTexCoords(vertex, u1)
	
	sfVertexArray_append(vertexArray, vertex)
	
	sfVertex_setPosition(vertex, [x + width, y])
	sfVertex_setTexCoords(vertex, u2)
	
	sfVertexArray_append(vertexArray, vertex)
	
	sfVertex_setPosition(vertex, [x + width, y + height])
	sfVertex_setTexCoords(vertex, u3)
	
	sfVertexArray_append(vertexArray, vertex)
	
	sfVertex_setPosition(vertex, [x, y + height])
	sfVertex_setTexCoords(vertex, u4)
	
	sfVertexArray_append(vertexArray, vertex)
end

func transformable()
	var transform = sfTransform_create()

	return {
		transform = transform,
		position = [0, 0],
		scale = [1, 1],
		rotation = 0,
		transformNeedUpdate = true,
		
		setPosition = transformable_setPosition,
		setScale = transformable_setScale,
		setRotation = transformable_setRotation
		
		getTransform = transformable_getTransform,
		
		moveBy = transformable_move,
		rotateBy = transformable_rotate,
		scaleBy = transformable_scale
	}
end

func transformable_setPosition(self, pos)
	self.position = pos
	self.transformNeedUpdate = true
end

func transformable_setScale(self, scale)
	self.scale = scale
	self.transformNeedUpdate = true
end

func transformable_setRotation(self, rotation)
	self.rotation = rotation
	self.transformNeedUpdate = true
end

func transformable_move(self, offset)
	self:setPosition([self.position[0] + offset[0], self.position[1] + offset[1]])
end

func transformable_rotate(self, angle)
	self:setRotation(self.rotation + angle)
end

func transformable_scale(self, factors)
	self:setScale([self.scale[0] + factors[0], self.scale[1] + factors[1]])
end

func transformable_getTransform(self)
	if self.transformNeedUpdate
		sfTransform_identity(self.transform)
		
		sfTransform_translate(self.transform, self.position)
		sfTransform_scale(self.transform, self.scale)
		sfTransform_rotate(self.transform, self.rotation)
		
		self.transformNeedUpdate = false
	end
	
	return self.transform
end

func sprite()
	var self = transformable()
	var vertexArray = sfVertexArray_create()
	var states = sfRenderStates_createDefault()
	
	self.texture = null
	self.vertexArray = vertexArray
	self.states = states
	self.textureRect = null
	self.color = [255, 255, 255, 255]
	self.needUpdateVertexArray = false
	
	self.setTexture = sprite_setTexture
	self.setTextureRect = sprite_setTextureRect
	self.setColor = sprite_setColor
	self.draw = sprite_draw
	
	return self
end

func sprite_setTexture(self, texture)
	self.texture = texture
	
	var size = sfTexture_getSize(texture)
	self.textureRect = [0, 0, size[0], size[1]]

	self.needUpdateVertexArray = true
end

func sprite_setTextureRect(self, textureRect)
	self.textureRect = textureRect
	self.needUpdateVertexArray = true
end

func sprite_setColor(self, color)
	self.color = color
	self.needUpdateVertexArray = true
end

func sprite_draw(self, window, states)
	sfRenderStates_setTransform(self.states, self:getTransform())
	sfRenderStates_setTexture(self.states, self.texture)
	
	if self.texture
		if self.needUpdateVertexArray
			sfVertexArray_clear(self.vertexArray)
			sfVertexArray_appendQuadEx(self.vertexArray, 0, 0, self.textureRect[2], self.textureRect[3], self.color, 
			[self.textureRect[0], self.textureRect[1]],
			[self.textureRect[0] + self.textureRect[2], self.textureRect[1]],
			[self.textureRect[0] + self.textureRect[2], self.textureRect[1] + self.textureRect[3]],
			[self.textureRect[0], self.textureRect[1] + self.textureRect[3]])
			
			self.needUpdateVertexArray = false
		end
		
		sfRenderWindow_drawVertexArray(window, self.vertexArray, self.states)
	end
end

func sprite_batch(window)
	var vertexArray = sfVertexArray_create()
	
	return {
		vertexArray = vertexArray,
		vertex = sfVertex_create(),
		texture = null,
		window = window,
		states = sfRenderStates_createDefault(),
		transform = sfTransform_create(),
		
		clear = sprite_batch_clear,
		flush = sprite_batch_flush,
		append = sprite_batch_append,
		appendEx = sprite_batch_appendEx,
		draw = sprite_batch_draw,
	}
end

func sprite_batch_clear(self)
	sfVertexArray_clear(self.vertexArray)
end

func sprite_batch_flush(self)
	sfVertexArray_setPrimitiveType(self.vertexArray, sfQuads)
	sfRenderStates_setTexture(self.states, self.texture)
	sfRenderWindow_drawVertexArray(self.window, self.vertexArray, self.states)
	sfVertexArray_clear(self.vertexArray)
end

func sprite_batch_append(self, x, y, texture)
	if self.texture != texture
		self.texture = texture
		if self.texture != null
			self:flush()
		end
	end
	
	var size = sfTexture_getSize(texture)
	sfVertexArray_appendQuad(self.vertexArray, size[0], size[1], [255, 255, 255, 255])
end

func sprite_batch_appendEx(self, x, y, texture, sx, sy, sw, sh, angle, scaleX, scaleY, ox, oy)
	if self.texture != texture
		self.texture = texture
		self:flush()
	end
	
	sfTransform_identity(self.transform)
	
	ox = ox * scaleX
	oy = oy * scaleY
	
	sfTransform_translate(self.transform, [x - ox, y - oy])
	sfTransform_translate(self.transform, [ox, oy])
	sfTransform_rotate(self.transform, angle)
	sfTransform_translate(self.transform, [-ox, -oy])
	sfTransform_scale(self.transform, [scaleX, scaleY])
	
	var p1 = sfTransform_transformPoint(self.transform, [0, 0])
	var p2 = sfTransform_transformPoint(self.transform, [sw, 0])
	var p3 = sfTransform_transformPoint(self.transform, [sw, sh])
	var p4 = sfTransform_transformPoint(self.transform, [0, sh])
	
	sfVertex_setColor(self.vertex, [255, 255, 255, 255])
	
	sfVertex_setPosition(self.vertex, p1)
	sfVertex_setTexCoords(self.vertex, [sx, sy])
	
	sfVertexArray_append(self.vertexArray, self.vertex)
	
	sfVertex_setPosition(self.vertex, p2)
	sfVertex_setTexCoords(self.vertex, [sx + sw, sy])
	
	sfVertexArray_append(self.vertexArray, self.vertex)
	
	sfVertex_setPosition(self.vertex, p3)
	sfVertex_setTexCoords(self.vertex, [sx + sw, sy + sh])
	
	sfVertexArray_append(self.vertexArray, self.vertex)
	
	sfVertex_setPosition(self.vertex, p4)
	sfVertex_setTexCoords(self.vertex, [sx, sy + sh])
	
	sfVertexArray_append(self.vertexArray, self.vertex)
end

func sprite_batch_draw(self, window, states)
	self.window = window
	self:flush()
end