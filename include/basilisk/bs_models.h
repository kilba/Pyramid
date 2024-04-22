#ifndef BS_MODELS_H
#define BS_MODELS_H

#include <bs_types.h>

void bs_pushModelBuffers();
void bs_pushArmatures();

/// <summary>
/// Get the current transformation of a joint, calculated with bs_updateArmature(...)
/// </summary>
/// <param name="armature"></param>
/// <param name="joint"></param>
bs_mat4 bs_jointTransformation(bs_Armature* armature, bs_Joint* joint);

/// <summary>
/// Get the current position of a joint, calculated with bs_updateArmature(...)
/// </summary>
/// <param name="armature"></param>
/// <param name="joint"></param>
bs_vec3 bs_jointPosition(bs_Armature* armature, bs_Joint* joint);

/// <summary>
/// Converts a position from local space to a joint's space.
/// </summary>
void bs_calculateJoint(bs_Armature* armature, bs_Joint* joint, bs_mat4 transformation, bs_mat4* destination);

/// <summary>
/// Gets the translation from a joint in an animation at a timestamp in the animation.
/// </summary>
bs_vec3 bs_interpolateTranslation(bs_AnimationJoint* animation_joint, float time);

/// <summary>
/// Loads a 3D model. Supported file types are: .glb, .gltf
/// </summary>
/// <param name="directory">- Directory to the model and the model's resources</param>
/// <param name="file_name">- Model's file name</param>
/// <returns>A new model object.</returns>
bs_Model bs_model(const char* directory, const char* file_name);

/// <summary>
/// Creates a new armature in the internal shader spaces, should be called during scene loading.
/// </summary>
/// <param name="armature"></param>
/// <param name="resting_anim">- Optional resting animation.</param>
/// <returns>An armature storage object used for updating the armature pose with bs_updateArmature().</returns>
bs_ArmatureStorage bs_pushArmature(bs_Armature* armature, bs_Animation* resting_anim);

/// <summary>
/// Updates an armature pose in the internal shader spaces from an animation and a timestamp in seconds.
/// </summary>
/// <param name="storage">- Armature storage object created from bs_pushArmature().</param>
/// <param name="animation">- Animation to sample from, can be most commonly fetched with bs_animFromName().</param>
/// <param name="time">- Time of the animation to fetch from.</param>
void bs_updateArmature(bs_ArmatureStorage storage, bs_Animation* animation, float time);

/// <summary>
/// Gets a joint in an armature.
/// </summary>
/// <param name="armature"></param>
/// <param name="name"></param>
/// <returns>The index of the joint or root if non existing.</returns>
bs_Joint* bs_boneFromName(bs_Armature* armature, const char* name);

/// <summary>
/// Searches for an animation in a model by name, avoid calling often.
/// </summary>
/// <param name="model"></param>
/// <param name="name"></param>
/// <returns>Pointer to the animation.</returns>
bs_Animation* bs_animation(bs_Model* model, const char* name);

/// <summary>
/// Searches for an armature in a model by name, avoid calling often.
/// </summary>
/// <param name="model"></param>
/// <param name="name"></param>
/// <returns>Pointer to the armature.</returns>
bs_Armature* bs_armature(bs_Model* model, const char* name);

/// <summary>
/// Searches for a mesh in a model by name, avoid calling often.
/// </summary>
/// <param name="model"></param>
/// <param name="name"></param>
/// <returns>Pointer to the mesh.</returns>
bs_Mesh* bs_meshFromName(bs_Model* model, const char *name);

/// <summary>
/// Searches for a mesh's index in a model by name, avoid calling often.
/// </summary>
/// <param name="model"></param>
/// <param name="name"></param>
/// <returns>The index of the mesh</returns>
int bs_meshIdxFromName(bs_Model* model, const char *name);

/// <summary>
/// Calculates and sets the "aabb" fields in bs_Model, bs_Mesh and bs_Prim.
/// </summary>
/// <param name="model"></param>
void bs_calculateModelBounds(bs_Model* model);

#endif // BS_MODELS_H