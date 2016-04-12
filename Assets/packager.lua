local lfs = require('lfs')
local gbk = require('gbk')
local argparse = require('argparse')

local args = {}
--local res_dir = 'D:\\ShadowPlay\\GameResources'
local res_dir = 'D:\\Projects\\p01\\Assets'
local cache_dir = 'D:\\ShadowPlay\\GameResources\\Cache'
local code_debug_dir = gbk.fromutf8('D:\\ShadowPlay\\程序\\PS4 Project\\ShadowPlay\\ORBIS_Debug')
local code_release_dir = gbk.fromutf8('D:\\ShadowPlay\\程序\\PS4 Project\\ShadowPlay\\ORBIS_Release')
local code_dir = code_release_dir
local dest_dir = 'O:\\10.0.0.150\\data\\app'
local zip_prog = 'resc.exe'
local tc_prog = 'texc.exe'
local src_filelist = {}
local AR_BASE_DIR
local AR_REV_DIR
local BASE_RES_NAME = 'base.res'
local PLATFORM = 'pc'

local FILTERS = {
  '%.lua$',
  '%.json$',
  '%.atlas$',
  '%.obj$',
  '%.mtl$',
  '%.ttf$',
  '%.ogv$',
  'Sound/Desktop/.*%.bank$',
  'Effects/Images/.*%.txt$',
}

local PS4_FILTERS = {
  '%.gnf$',
  '%.vsb$',
  '%.fsb$',
}
  
local PC_FILTERS = {
  '%.png$',
  '%.jpg$',
  '%.dds$',
  '%.tga$',
  '%.DDS$',
  'Shaders/DX/.*%.vsh$',
  'Shaders/DX/.*%.fsh$',
  'Shaders/GL/.*%.vsh$',
  'Shaders/GL/.*%.fsh$',
}

local parse_args
local gather_filelist
local do_archives
local do_textures
local do_package
local do_exe
local main

parse_args = function ()
  local arg_parser = argparse()
  arg_parser:command_target('command')
  local pub_cmd = arg_parser:command('publish p')
  local archive_cmd = arg_parser:command('archive a')
  local texture_cmd = arg_parser:command('texture t')
  pub_cmd:option('-o --output-dir', 'output directory', dest_dir)
  pub_cmd:option('-r --revision', 'publish revison', 0)
  pub_cmd:flag('-d --debug', 'publish debug code')
  pub_cmd:flag('-e --exe-only', 'publish code only')
  pub_cmd:mutex(
    pub_cmd:flag('--ps4', 'PS4 version asset'),
    pub_cmd:flag('--pc', 'PC version asset')
  )
  archive_cmd:option('-r --revision', 'publish revison', 0)
  archive_cmd:mutex(
    archive_cmd:flag('--ps4', 'PS4 version asset'),
    archive_cmd:flag('--pc', 'PS4 version asset')
  )
  texture_cmd:mutex(
    texture_cmd:flag('--ps4', 'PS4 version asset'),
    texture_cmd:flag('--pc', 'PS4 version asset')
  )
  
  args = arg_parser:parse()
  
  dest_dir = args.output_dir
  if args.debug then
    code_dir = code_debug_dir
  end
  
  local extra_filters = {}
  if args.ps4 then
    PLATFORM = 'ps4'
  elseif args.pc then
    PLATFORM = 'pc'
  else
    print('Default to PS4...')
    args.ps4 = true
	  PLATFORM = 'ps4'
  end
  if args.ps4 then
    extra_filters = PS4_FILTERS
  elseif args.pc then
    extra_filters = PC_FILTERS
  end
  for _, f in ipairs(extra_filters) do
    table.insert(FILTERS, f)
  end

  AR_BASE_DIR = string.format('Cache/%s-base', PLATFORM)
  if not args.revision or args.revision == 0 then
    AR_REV_DIR = AR_BASE_DIR
  else
    AR_REV_DIR = string.format('Cache/%s-patch%03d', PLATFORM, args.revision)
  end
end

local walk_dir
walk_dir = function (dir)
  local prefix = dir .. '/'
  for f in lfs.dir(dir) do
    if f ~= '.' and f ~= '..' then
      local attr = lfs.attributes(prefix .. f)
      local m = attr.mode
      if m == "directory" then
        walk_dir(prefix .. f)
      elseif m == "file" then
        table.insert(src_filelist, prefix .. f)
      end
    end
  end
end

local filter_filelist = function ()
  local l = {}
  for _, path in ipairs(src_filelist) do
    for _, filter in ipairs(FILTERS) do
      if path:match(filter) then
        table.insert(l, path)
        break
      end
    end
  end
  src_filelist = l
end

do_textures = function ()
  if args.command == 'texture' then
    src_filelist = {}
    for f in lfs.dir(res_dir) do
      if f ~= '.' and f ~= '..' and f ~= 'Cache' and f ~= 'Table' and string.sub(f, 1, 1) ~= '_' then
        local attr = lfs.attributes(f)
        if attr.mode == "directory" then
          walk_dir(f)
        end
      end
    end
    local png_list = {}
    for _, path in ipairs(src_filelist) do
      if path:match('%.png$') or path:match('%.tga$') then
        png_list[#png_list + 1] = path
      elseif path:match('%.DDS$') then
        print("Renaming 'DDS' to 'dds':", path)
        local old_path = string.gsub(path, '/', '\\')
        local new_path = string.gsub(path, '.*/', '')
        new_path = string.sub(new_path, 1, #new_path - 4) .. '.dds'
        local cmd = string.format('rename %s %s', old_path, new_path)
        local ok = os.execute(cmd)
        if not ok then
          print('Failed to run cmd: ', cmd)
          os.exit(-1)
        end
      end
    end
    for _, path in ipairs(png_list) do
      local texture_path
      if args.ps4 then
        texture_path = string.sub(path, 1, #path - 4) .. '.gnf'
      else
        texture_path = string.sub(path, 1, #path - 4) .. '.dds'
      end
      local orig_attr = lfs.attributes(path)
      local new_attr = lfs.attributes(texture_path)
      if not new_attr or orig_attr.modification > new_attr.modification then
        print('Compressing ', path)
        local cmd = string.format('%s %s -m %s', tc_prog, args.ps4 and '--ps4' or '--pc', path)
        local ok = os.execute(cmd)
        if not ok then
          print('Failed to run cmd: ', cmd)
          --os.exit(-1);
        end
      end
    end
  end
end

gather_filelist = function ()
  src_filelist = {}
  for f in lfs.dir(res_dir) do
    if f ~= '.' and f ~= '..' and f ~= 'Cache' and f ~= 'Table' and string.sub(f, 1, 1) ~= '_' then
      local attr = lfs.attributes(f)
      if attr.mode == "directory" then
        walk_dir(f)
      end
    end
  end
  filter_filelist()
  
  lfs.mkdir(AR_REV_DIR)
  local f = io.open(AR_REV_DIR .. '/filelist.txt', 'wb')
  for _, path in ipairs(src_filelist) do
    f:write(gbk.toutf8(path), '\n')
  end
  f:close()
end

do_archives = function ()
  if args.revision == 0 then
    local cmd = string.format('%s %s/filelist.txt %s', zip_prog, AR_REV_DIR, AR_REV_DIR)
    local ok = os.execute(cmd)
    if not ok then
      print('Failed to run cmd: ', cmd)
      os.exit(-1);
    end
  else
    local cmd = string.format('%s %s/filelist.txt %s %s', zip_prog, AR_REV_DIR, AR_BASE_DIR, AR_REV_DIR)
    local ok = os.execute(cmd)
    if not ok then
      print('Failed to run cmd: ', cmd)
      os.exit(-1);
    end
  end

  if args.command == 'publish' then
    -- copy to destionation directory
    lfs.mkdir(dest_dir .. '\\GameResources')
    local index_fname = 'resource.idx'
    local manifest_fname = 'manifest.txt'
    local names = {BASE_RES_NAME, index_fname, manifest_fname}
    if args.revision ~= 0 then
      table.insert(names, string.format('patch%03d.res', args.revision))
    end
    for _, path in ipairs(names) do
      print('Copying ' .. path .. ' ...')
      local src_dir = string.gsub(AR_REV_DIR, '/', '\\')
      if path == BASE_RES_NAME then
        src_dir = string.gsub(AR_BASE_DIR, '/', '\\')
      end
      local cmd = string.format('xcopy %s\\%s %s\\GameResources /Y /D /Q', src_dir, path, dest_dir)
      local ok = os.execute(cmd)
      if not ok then
        print('Failed!!')
        os.exit(-1)
      end
    end
  end
end


do_exe = function ()
  if args.command == 'publish' then
    -- copy ShadowPlay.elf
    print('Copying ShadowPlay.elf ...')
    local ok = os.execute(string.format('xcopy "%s\\ShadowPlay.elf" %s\\eboot.bin* /Y /D /Q', code_dir, dest_dir))
    if not ok then
      print('Failed!!')
      os.exit(-1)
    end
  end
end

main = function ()
  lfs.chdir(res_dir)
  if not args.exe_only then
    do_textures()
    do return end
    gather_filelist()
    do_archives()
  end
  do_exe()
  print('---- Succeeds -----')
  os.execute('pause')
  os.exit(0)
end

parse_args()
main()
