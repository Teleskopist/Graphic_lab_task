#pragma once

#include <vector>
#include <queue>
#include <string>
#include "blk/blk.h"

enum class Cmd
{
  NO_OP,
  INIT,
  EXIT,
  LOAD_SCENE,
  SAVE_SCENE,
  CONVERT_SCENE,
  SET_ENGINE_SETTINGS,
  SET_RENDER_SETTINGS,
  SET_CAMERA,
  TAKE_SCREENSHOT,
  INIT_RAYTRACING,
  INIT_RASTERIZATION,
  REFRESH_RASTERIZATION,
  RENDER_FRAME,
  COMPARE_FRAMES,
  SET_RENDER_DEVICE,
  CUT_VTK_DATASET,
  SET_AA,
  RECORD_TRAJECTORY,
  PLAYBACK_TRAJECTORY,
  SET_MINIMAL_UI,
  PRELOAD_VTK_DATASET,
  SET_DENOISER,
  SET_DENOISER_SIGMA_R,
  SET_DENOISER_SIGMA_S,
  SET_LIGHTS,
  SET_CAMERA_LIGHT,
  SET_DENOISER_KERNEL_SIZE,
  SET_LIVE_METRICS_CALCULATION,
  CHANGE_VISIBILITY,
  CAST_DEBUG_RAY,
};

class CommandBuffer
{
public:
  struct Command
  {
    Cmd type;
    unsigned long id;
    Block args;
    Command(Cmd _type, unsigned long _id)
    {
      type = _type;
      id = _id;
      args = Block();
      args.set_enum("cmd_code", "Cmd", (int)_type);
    }
    Command(Cmd _type, unsigned long _id, const Block &_args)
    {
      type = _type;
      id = _id;
      args = Block();
      args.copy(&_args);
      args.set_enum("cmd_code", "Cmd", (int)_type);
    }
  };
  void push(Cmd cmd_type)
  {
    commands.emplace(cmd_type, next_command_id);
    next_command_id++;
  }
  void push(Cmd cmd_type, const Block &args)
  {
    commands.emplace(cmd_type, next_command_id, args);
    next_command_id++;
  }
  bool empty()
  {
    return commands.empty();
  }
  Command &front()
  {
    return commands.front();
  }
  void pop()
  {
    commands.pop();
  }
  void log()
  {
    cmd_log.add_block("cmd_" + std::to_string(commands.front().id), &(commands.front().args));
    save_block_to_file(std::string("./saves/last_cmd_log.blk"), cmd_log);
  }
  CommandBuffer() = default;
  ~CommandBuffer() = default;

private:
  std::queue<Command> commands;
  Block cmd_log;
  unsigned next_command_id = 0;
};