--[[

]]

 Sfxr = require 'cppSfxr'

-- Load some default values for our rectangle.
function love.load()
  Sfxr:init()
  Sfxr:create(Sfxr.Sound.EXPLOSION)
  source = love.audio.newSource(Sfxr:soundData())
end

-- Increase the size of the rectangle every frame.
function love.update(dt)
end

-- Draw a coloured rectangle.
function love.draw()

end

function love.keypressed( key, scancode, isrepeat )

end
