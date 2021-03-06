// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

/** @file
 * @brief Class @ref esp::physics::RigidObject, enum @ref
 * esp::physics::MotionType, enum @ref esp::physics::RigidObjectType, struct
 * @ref VelocityControl
 */

#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Reference.h>
#include "esp/assets/Asset.h"
#include "esp/assets/Attributes.h"
#include "esp/assets/BaseMesh.h"
#include "esp/assets/GenericInstanceMeshData.h"
#include "esp/assets/MeshData.h"
#include "esp/assets/ResourceManager.h"
#include "esp/core/esp.h"
#include "esp/scene/SceneNode.h"

namespace esp {

namespace assets {
class PhysicsObjectAttributes;
class ResourceManager;
}  // namespace assets
namespace physics {

/**
@brief Motion type of a @ref RigidObject.
Defines its treatment by the simulator and operations which can be performed on
it.
*/
enum class MotionType {
  /**
   * Refers to an error (such as a query to non-existing object).
   */
  ERROR_MOTIONTYPE,

  /**
   * The object is not expected to move and should not allow kinematic updates.
   * Likely treated as static collision geometry. See @ref
   * RigidObjectType::SCENE.
   */
  STATIC,

  /**
   * The object is expected to move kinematically, but is not simulated. Default
   * behavior of @ref RigidObject with no physics simulator defined.
   */
  KINEMATIC,

  /**
   * The object is simulated and can, but should not be, updated kinematically .
   * Default behavior of @ref RigidObject with a physics simulator defined. See
   * @ref BulletRigidObject.
   */
  DYNAMIC

};

/**
@brief Category of a @ref RigidObject. Defines treatment of the object in @ref
PhysicsManager. Also denotes the status of an object as initialized or not.
*/
enum class RigidObjectType {
  /**
   * The object is not initialized yet. Set as default on construction.
   */
  NONE,

  /**
   * The object is a @ref MotionType::STATIC scene collision geometry.
   * See @ref PhysicsManager::addScene.
   */
  SCENE,

  /**
   * The object is a standard rigid object and should be tracked in @ref
   * PhysicsManager::existingObjects_.
   * @ref PhysicsManager::existingObjects_. See @ref PhysicsManager::addObject.
   */
  OBJECT

};

/**@brief Convenience struct for applying constant velocity control to a rigid
 * body. */
struct VelocityControl {
 public:
  virtual ~VelocityControl(){};

  /**@brief Constant linear velocity. */
  Magnum::Vector3 linVel;
  /**@brief Constant angular velocity. */
  Magnum::Vector3 angVel;
  /**@brief Whether or not to set linear control velocity before stepping. */
  bool controllingLinVel = false;
  /**@brief Whether or not to set linear control velocity in local space.
   *
   * Useful for commanding actions such as "forward", or "strafe".
   */
  bool linVelIsLocal = false;

  /**@brief Whether or not to set angular control velocity before stepping. */
  bool controllingAngVel = false;

  /**@brief Whether or not to set angular control velocity in local space.
   *
   * Useful for commanding actions such as "roll" and "yaw".
   */
  bool angVelIsLocal = false;

  /**
   * @brief Compute the result of applying constant control velocities to the
   * provided object transform.
   *
   * Default implementation uses explicit Euler integration.
   * @param dt The discrete timestep over which to integrate.
   * @param objectTransformation The initial state of the object before applying
   * velocity control.
   * @return The new state of the object after applying velocity control over
   * dt.
   */
  virtual Magnum::Matrix4 integrateTransform(
      const float dt,
      const Magnum::Matrix4& objectTransform);

  ESP_SMART_POINTERS(VelocityControl)
};

/**
@brief An AbstractFeature3D representing an individual rigid object instance
attached to a SceneNode, updating its state through simulation. This may be a
@ref MotionType::STATIC scene collision geometry or an object of any @ref
MotionType which can interact with other members of a physical world. Must have
a collision mesh. By default, a RigidObject is @ref MotionType::KINEMATIC
without an underlying simulator implementation. Derived classes can be used to
introduce specific implementations of dynamics.
*/
class RigidObject : public Magnum::SceneGraph::AbstractFeature3D {
 public:
  /**
   * @brief Constructor for a @ref RigidObject.
   * @param rigidBodyNode The @ref scene::SceneNode this feature will be
   * attached to.
   */
  RigidObject(scene::SceneNode* rigidBodyNode);

  /**
   * @brief Virtual destructor for a @ref RigidObject.
   */
  virtual ~RigidObject(){};

  /**
   * @brief Get the scene node being attached to.
   */
  scene::SceneNode& node() { return object(); }
  const scene::SceneNode& node() const { return object(); }

  // Overloads to avoid confusion
  scene::SceneNode& object() {
    return static_cast<scene::SceneNode&>(
        Magnum::SceneGraph::AbstractFeature3D::object());
  }
  const scene::SceneNode& object() const {
    return static_cast<const scene::SceneNode&>(
        Magnum::SceneGraph::AbstractFeature3D::object());
  }

  /**
   * @brief Initializes this @ref RigidObject as static scene geometry. See @ref
   * PhysicsManager::staticSceneObject_. Sets @ref rigidObjectType_ to @ref
   * RigidObjectType::SCENE.
   * @param physicsSceneAttributes The template structure defining relevant
   * phyiscal parameters for the physical scene.
   * @param meshGroup The collision mesh data for the scene.
   * @return true if initialized successfully, false otherwise.
   */
  bool initializeScene(
      const assets::ResourceManager& resMgr,
      const assets::PhysicsSceneAttributes::ptr physicsSceneAttributes,
      const std::vector<assets::CollisionMeshData>& meshGroup);

  /**
   * @brief Initializes this @ref RigidObject as a potentially moveable object.
   * Sets @ref rigidObjectType_ to @ref RigidObjectType::OBJECT.
   * @param physicsObjectAttributes The template structure defining relevant
   * phyiscal parameters for the object. See @ref
   * esp::assets::ResourceManager::physicsObjectLibrary_.
   * @param meshGroup The collision mesh data for the object.
   * @return true if initialized successfully, false otherwise.
   */
  bool initializeObject(
      const assets::ResourceManager& resMgr,
      const assets::PhysicsObjectAttributes::ptr physicsObjectAttributes,
      const std::vector<assets::CollisionMeshData>& meshGroup);

  /**
   * @brief Finalize this object with any necessary post-creation processes.
   */
  virtual void finalizeObject() {}

  /**
   * @brief Check whether object is being actively simulated, or sleeping.
   * Kinematic objects are always active, but derived dynamics implementations
   * may not be.
   * @return true if active, false otherwise.
   */
  virtual bool isActive();

  /**
   * @brief Set an object as being actively simulated rather than sleeping.
   * Kinematic objects are always active, but derived dynamics implementations
   * may not be.
   */
  virtual void setActive(){};

  /**
   * @brief Set the @ref MotionType of the object. If the object is @ref
   * ObjectType::SCENE it can only be @ref MotionType::STATIC. If the object is
   * @ref ObjectType::OBJECT is can also be set to @ref MotionType::KINEMATIC.
   * Only if a dervied @ref PhysicsManager implementing dynamics is in use can
   * the object be set to @ref MotionType::DYNAMIC.
   * @param mt The desirved @ref MotionType.
   * @return true if successfully set, false otherwise.
   */
  virtual bool setMotionType(MotionType mt);

  /**
   * @brief Get the @ref MotionType of the object. See @ref setMotionType.
   * @return The object's current @ref MotionType.
   */
  MotionType getMotionType() { return objectMotionType_; };

  /**
   * @brief Shift the object's local origin by translating all children of this
   * object's SceneNode.
   * @param shift The translation to apply to object's children.
   */
  virtual void shiftOrigin(const Magnum::Vector3& shift);

  /**
   * @brief Shift the object's local origin to be coincident with the center of
   * it's bounding box, @ref cumulativeBB_. See @ref shiftOrigin.
   */
  void shiftOriginToBBCenter();

  /**
   * @brief Apply a force to an object through a dervied dynamics
   * implementation. Does nothing for @ref MotionType::STATIC and @ref
   * MotionType::KINEMATIC objects.
   * @param force The desired force on the object in the global coordinate
   * system.
   * @param relPos The desired location of force application in the global
   * coordinate system relative to the object's center of mass.
   */
  virtual void applyForce(const Magnum::Vector3& force,
                          const Magnum::Vector3& relPos);

  /**
   * @brief Apply an impulse to an object through a dervied dynamics
   * implementation. Directly modifies the object's velocity without requiring
   * integration through simulation. Does nothing for @ref MotionType::STATIC
   * and @ref MotionType::KINEMATIC objects.
   * @param impulse The desired impulse on the object in the global coordinate
   * system.
   * @param relPos The desired location of impulse application in the global
   * coordinate system relative to the object's center of mass.
   */
  virtual void applyImpulse(const Magnum::Vector3& impulse,
                            const Magnum::Vector3& relPos);

  /**
   * @brief Apply an internal torque to an object through a dervied dynamics
   * implementation. Does nothing for @ref MotionType::STATIC and @ref
   * MotionType::KINEMATIC objects.
   * @param torque The desired torque on the object in the local coordinate
   * system.
   */
  virtual void applyTorque(const Magnum::Vector3& torque);

  /**
   * @brief Apply an internal impulse torque to an object through a dervied
   * dynamics implementation. Does nothing for @ref MotionType::STATIC and @ref
   * MotionType::KINEMATIC objects.
   * @param impulse The desired impulse torque on the object in the local
   * coordinate system. Directly modifies the object's angular velocity without
   * requiring integration through simulation.
   */
  virtual void applyImpulseTorque(const Magnum::Vector3& impulse);

  /**
   * @brief Virtual linear velocity setter for an object.
   *
   * Does nothing for default @ref MotionType::KINEMATIC or @ref
   * MotionType::STATIC objects.
   * @param linVel Linear velocity to set.
   */
  virtual void setLinearVelocity(
      CORRADE_UNUSED const Magnum::Vector3& linVel){};

  /**
   * @brief Virtual angular velocity setter for an object.
   *
   * Does nothing for default @ref MotionType::KINEMATIC or @ref
   * MotionType::STATIC objects.
   * @param angVel Angular velocity vector corresponding to world unit axis
   * angles.
   */
  virtual void setAngularVelocity(
      CORRADE_UNUSED const Magnum::Vector3& angVel){};

  /**
   * @brief Virtual linear velocity getter for an object.
   *
   * Returns zero for default @ref MotionType::KINEMATIC or @ref
   * MotionType::STATIC objects.
   * @return Linear velocity of the object.
   */
  virtual Magnum::Vector3 getLinearVelocity() const {
    return Magnum::Vector3();
  };

  /**
   * @brief Virtual angular velocity getter for an object.
   *
   * Returns zero for default @ref MotionType::KINEMATIC or @ref
   * MotionType::STATIC objects.
   * @return Angular velocity vector corresponding to world unit axis angles.
   */
  virtual Magnum::Vector3 getAngularVelocity() const {
    return Magnum::Vector3();
  };

  /**@brief Retrieves a reference to the VelocityControl struct for this object.
   */
  VelocityControl::ptr getVelocityControl() { return velControl_; };

  // ==== Transformations ===

  /** @brief Set the 4x4 transformation matrix of the object kinematically.
   * Calling this during simulation of a @ref MotionType::DYNAMIC object is not
   * recommended.
   * @param transformation The desired 4x4 transform of the object.
   */
  virtual void setTransformation(const Magnum::Matrix4& transformation);

  /** @brief Set the 3D position of the object kinematically.
   * Calling this during simulation of a @ref MotionType::DYNAMIC object is not
   * recommended.
   * @param vector The desired 3D position of the object.
   */
  virtual void setTranslation(const Magnum::Vector3& vector);

  /** @brief Set the orientation of the object kinematically.
   * Calling this during simulation of a @ref MotionType::DYNAMIC object is not
   * recommended.
   * @param quaternion The desired orientation of the object.
   */
  virtual void setRotation(const Magnum::Quaternion& quaternion);

  /** @brief Reset the transformation of the object.
   * !!NOT IMPLEMENTED!!
   */
  virtual void resetTransformation();

  /** @brief Modify the 3D position of the object kinematically by translation.
   * Calling this during simulation of a @ref MotionType::DYNAMIC object is not
   * recommended.
   * @param vector The desired 3D vector by which to translate the object.
   */
  virtual void translate(const Magnum::Vector3& vector);

  /** @brief Modify the 3D position of the object kinematically by translation
   * with a vector defined in the object's local coordinate system. Calling this
   * during simulation of a @ref MotionType::DYNAMIC object is not recommended.
   * @param vector The desired 3D vector in the object's ocal coordiante system
   * by which to translate the object.
   */
  virtual void translateLocal(const Magnum::Vector3& vector);

  /** @brief Modify the orientation of the object kinematically by applying an
   * axis-angle rotation to it. Calling this during simulation of a @ref
   * MotionType::DYNAMIC object is not recommended.
   * @param angleInRad The angle of rotation in radians.
   * @param normalizedAxis The desired unit vector axis of rotation.
   */
  virtual void rotate(const Magnum::Rad angleInRad,
                      const Magnum::Vector3& normalizedAxis);

  /** @brief Modify the orientation of the object kinematically by applying an
   * axis-angle rotation to it in the local coordinate system. Calling this
   * during simulation of a @ref MotionType::DYNAMIC object is not recommended.
   * @param angleInRad The angle of rotation in radians.
   * @param normalizedAxis The desired unit vector axis of rotation in the local
   * coordinate system.
   */
  virtual void rotateLocal(const Magnum::Rad angleInRad,
                           const Magnum::Vector3& normalizedAxis);

  /** @brief Modify the orientation of the object kinematically by applying a
   * rotation to it about the global X axis. Calling this during simulation of a
   * @ref MotionType::DYNAMIC object is not recommended.
   * @param angleInRad The angle of rotation in radians.
   */
  virtual void rotateX(const Magnum::Rad angleInRad);

  /** @brief Modify the orientation of the object kinematically by applying a
   * rotation to it about the global Y axis. Calling this during simulation of a
   * @ref MotionType::DYNAMIC object is not recommended.
   * @param angleInRad The angle of rotation in radians.
   */
  virtual void rotateY(const Magnum::Rad angleInRad);

  /** @brief Modify the orientation of the object kinematically by applying a
   * rotation to it about the global Z axis. Calling this during simulation of a
   * @ref MotionType::DYNAMIC object is not recommended.
   * @param angleInRad The angle of rotation in radians.
   */
  virtual void rotateZ(const Magnum::Rad angleInRad);

  /** @brief Modify the orientation of the object kinematically by applying a
   * rotation to it about the local X axis. Calling this during simulation of a
   * @ref MotionType::DYNAMIC object is not recommended.
   * @param angleInRad The angle of rotation in radians.
   */
  virtual void rotateXLocal(const Magnum::Rad angleInRad);

  /** @brief Modify the orientation of the object kinematically by applying a
   * rotation to it about the local Y axis. Calling this during simulation of a
   * @ref MotionType::DYNAMIC object is not recommended.
   * @param angleInRad The angle of rotation in radians.
   */
  virtual void rotateYLocal(const Magnum::Rad angleInRad);

  /** @brief Modify the orientation of the object kinematically by applying a
   * rotation to it about the local Z axis. Calling this during simulation of a
   * @ref MotionType::DYNAMIC object is not recommended.
   * @param angleInRad The angle of rotation in radians.
   */
  virtual void rotateZLocal(const Magnum::Rad angleInRad);

  // ==== Getter/Setter functions ===
  //! For kinematic objects they are dummies, for dynamic objects
  //! implemented in physics-engine specific ways

  /** @brief Get the mass of the object. Only used for dervied dynamic
   * implementations of @ref RigidObject.
   * @return The mass of the object.
   */
  virtual double getMass() { return 0.0; }

  /** @brief Get the scale of the object set during initialization.
   * @return The scaling for the object relative to its initially loaded meshes.
   */
  virtual Magnum::Vector3 getScale() {
    return initializationAttributes_->getScale();
  }

  /** @brief Get the scalar friction coefficient of the object. Only used for
   * dervied dynamic implementations of @ref RigidObject.
   * @return The scalar friction coefficient of the object.
   */
  virtual double getFrictionCoefficient() { return 0.0; }

  /** @brief Get the scalar coefficient of restitution  of the object. Only used
   * for dervied dynamic implementations of @ref RigidObject.
   * @return The scalar coefficient of restitution  of the object.
   */
  virtual double getRestitutionCoefficient() { return 0.0; }

  /** @brief Get the scalar linear damping coefficient of the object. Only used
   * for dervied dynamic implementations of @ref RigidObject.
   * @return The scalar linear damping coefficient of the object.
   */
  virtual double getLinearDamping() { return 0.0; }

  /** @brief Get the scalar angular damping coefficient of the object. Only used
   * for dervied dynamic implementations of @ref RigidObject.
   * @return The scalar angular damping coefficient of the object.
   */
  virtual double getAngularDamping() { return 0.0; }

  /** @brief Get the center of mass (COM) of the object.
   * @return Object 3D center of mass in the global coordinate system.
   * @todo necessary for @ref MotionType::KINEMATIC?
   */
  virtual Magnum::Vector3 getCOM();

  /** @brief Get the diagonal of the inertia matrix for an object.
   * If an object is aligned with its principle axii of inertia, the 3x3 inertia
   * matrix can be reduced to a diagonal. See @ref
   * RigidObject::setInertiaVector.
   * @return The diagonal of the object's inertia matrix.
   */
  virtual Magnum::Vector3 getInertiaVector();

  /** @brief Get the 3x3 inertia matrix for an object.
   * @return The object's 3x3 inertia matrix.
   * @todo provide a setter for the full 3x3 inertia matrix. Not all
   * implementations will provide this option.
   */
  virtual Magnum::Matrix3 getInertiaMatrix();

  /** @brief Set the mass of the object. Only used for dervied dynamic
   * implementations of @ref RigidObject.
   * @param mass The new mass of the object.
   */
  virtual void setMass(CORRADE_UNUSED const double mass){};

  /** @brief Set the center of mass (COM) of the object.
   * @param COM Object 3D center of mass in the local coordinate system.
   * @todo necessary for @ref MotionType::KINEMATIC?
   */
  virtual void setCOM(CORRADE_UNUSED const Magnum::Vector3& COM){};

  /** @brief Set the diagonal of the inertia matrix for the object.
   * If an object is aligned with its principle axii of inertia, the 3x3 inertia
   * matrix can be reduced to a diagonal.
   * @param inertia The new diagonal for the object's inertia matrix.
   */
  virtual void setInertiaVector(
      CORRADE_UNUSED const Magnum::Vector3& inertia){};

  /** @brief Set the scalar friction coefficient of the object. Only used for
   * dervied dynamic implementations of @ref RigidObject.
   * @param frictionCoefficient The new scalar friction coefficient of the
   * object.
   */
  virtual void setFrictionCoefficient(
      CORRADE_UNUSED const double frictionCoefficient){};

  /** @brief Set the scalar coefficient of restitution of the object. Only used
   * for dervied dynamic implementations of @ref RigidObject.
   * @param restitutionCoefficient The new scalar coefficient of restitution of
   * the object.
   */
  virtual void setRestitutionCoefficient(
      CORRADE_UNUSED const double restitutionCoefficient){};

  /** @brief Set the scalar linear damping coefficient of the object. Only used
   * for dervied dynamic implementations of @ref RigidObject.
   * @param linDamping The new scalar linear damping coefficient of the object.
   */
  virtual void setLinearDamping(CORRADE_UNUSED const double linDamping){};

  /** @brief Set the scalar angular damping coefficient for the object. Only
   * used for dervied dynamic implementations of @ref RigidObject.
   * @param angDamping The new scalar angular damping coefficient for the
   * object.
   */
  virtual void setAngularDamping(CORRADE_UNUSED const double angDamping){};

  /**
   * @brief Get the template used to initialize this object.
   *
   * PhysicsObjectAttributes templates are expected to be changed between
   * instances of objects.
   * @return The initialization settings of this object instance.
   */
  const assets::PhysicsObjectAttributes::ptr getInitializationAttributes()
      const {
    return initializationAttributes_;
  };

  /** @brief Store whatever object attributes you want here! */
  esp::core::Configuration attributes_;

  //! The @ref SceneNode of a bounding box debug drawable. If nullptr, BB
  //! drawing is off. See @ref toggleBBDraw().
  scene::SceneNode* BBNode_ = nullptr;

  /**
   * @brief All Drawable components are children of this node.
   */
  scene::SceneNode* visualNode_ = nullptr;

 protected:
  /**
   * @brief Convenience variable: specifies a constant control velocity (linear
   * | angular) applied to the rigid body before each step.
   */
  VelocityControl::ptr velControl_;

  /** @brief The @ref MotionType of the object. Determines what operations can
   * be performed on this object. */
  MotionType objectMotionType_;

  /** @brief The @ref RigidObjectType of the object. Identifies what role the
   * object plays in the phyiscal world. A value of @ref RigidObjectType::NONE
   * identifies the object as uninitialized.*/
  RigidObjectType rigidObjectType_ = RigidObjectType::NONE;

  /**
   * @brief Saved attributes when the object was initialized.
   */
  assets::PhysicsObjectAttributes::ptr initializationAttributes_;

 private:
  /**
   * @brief Finalize the initialization of this @ref RigidObject as static scene
   * geometry.  This is overridden by inheriting objects
   * @param resMgr Reference to resource manager, to access relevant components
   * pertaining to the scene object
   * @param physicsSceneAttributes The template structure defining relevant
   * phyiscal parameters for the physical scene.
   * @param meshGroup The collision mesh data for the scene.
   * @return true if initialized successfully, false otherwise.
   */
  virtual bool initializeSceneFinalize(
      const assets::ResourceManager& resMgr,
      const assets::PhysicsSceneAttributes::ptr physicsSceneAttributes,
      const std::vector<assets::CollisionMeshData>& meshGroup);

  /**
   * @brief Finalize initialization of this @ref RigidObject as a potentially
   * moveable object. This is overridden by inheriting objects
   * @param resMgr Reference to resource manager, to access relevant components
   * pertaining to the scene object
   * @param physicsObjectAttributes The template structure defining relevant
   * phyiscal parameters for the object. See @ref
   * esp::assets::ResourceManager::physicsObjectLibrary_.
   * @param meshGroup The collision mesh data for the object.
   * @return true if initialized successfully, false otherwise.
   */
  virtual bool initializeObjectFinalize(
      const assets::ResourceManager& resMgr,
      const assets::PhysicsObjectAttributes::ptr physicsObjectAttributes,
      const std::vector<assets::CollisionMeshData>& meshGroup);

 protected:
  /** @brief Used to synchronize other simulator's notion of the object state
   * after it was changed kinematically. Called automatically on kinematic
   * updates.*/
  virtual void syncPose();

  ESP_SMART_POINTERS(RigidObject)
};

}  // namespace physics
}  // namespace esp
