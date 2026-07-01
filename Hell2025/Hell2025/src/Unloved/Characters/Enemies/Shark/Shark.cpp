#include "Shark.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Hell/Math/LineMath.h"
#include "Hell/Math/Math.h"
#include "World/LegacyWorld.h"
#include "Unloved/ObjectId.h"
#include "Unloved/World/World.h"

#include "Unloved/Systems/Ocean/Ocean.h"
#include "Hell/Input.h"
#include "Hell/Audio.h"
namespace Audio = Hell::Audio;
namespace Input = Hell::Input;

#include "Renderer/Renderer.h" // TODO: get me out of here

namespace Unloved {

    inline glm::vec3 NormalizeXZOr(const glm::vec3& v, const glm::vec3& fallback) {
        glm::vec3 out = v;
        out.y = 0.0f;
        float lenSq = glm::dot(out, out);
        if (lenSq > 0.000001f) return out / std::sqrt(lenSq);
        return fallback;
    }

    std::vector<glm::vec3> GetCirclePoints(const glm::vec3& center, int segments, float radius) {
        std::vector<glm::vec3> pts;
        pts.reserve(segments);
        const float PI = 3.14159265358979323846f;
        float dTheta = 2.0f * PI / segments;
        for (int i = 0; i < segments; ++i) {
            float theta = i * dTheta;
            pts.emplace_back(
                center.x + std::cos(theta) * radius,
                center.y,
                center.z + std::sin(theta) * radius
            );
        }
        return pts;
    }

    Shark::Shark(uint64_t id, const SharkCreateInfo& createInfo, const SpawnOffset& spawnOffset) {
        m_objectId = id;
        m_createInfo = createInfo;
        m_createInfo.position += spawnOffset.translation;

        Init();
    }

    void Shark::Init() {
        const glm::vec3& initialPosition = m_createInfo.position;

        m_yHeight = Ocean::GetOceanOriginY();

        g_animatedGameObjectObjectId = LegacyWorld::CreateAnimatedGameObject();

        PhysicsFilterData filterData;
        filterData.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
        filterData.collisionGroup = CollisionGroup::RAGDOLL_ENEMY;
        filterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | CHARACTER_CONTROLLER | RAGDOLL_ENEMY);

        Unloved::AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
        animatedGameObject->SetSkinnedModel("Shark");
        animatedGameObject->SetName("GreatestGreatWhiteShark");
        animatedGameObject->SetAllMeshMaterials("Shark");
        animatedGameObject->PlayAndLoopAnimation("MainLayer", "Shark_Swim", 1.0f);

        animatedGameObject->SetScale(0.01);
        animatedGameObject->SetPosition(glm::vec3(0, 0, 0));

        m_RagdollId = Hell::Physics::SpawnRagdoll(glm::vec3(0.0f), glm::vec3(0.0f), "Shark", m_objectId, filterData);

        animatedGameObject->SetRagdollId(m_RagdollId);

        SkinnedModel* skinnedModel = animatedGameObject->GetSkinnedModel();
        std::vector<Node>& nodes = skinnedModel->m_nodes;
        std::map<std::string, unsigned int>& boneMapping = skinnedModel->m_boneMapping;

        // Extract spine positions
        int nodeCount = nodes.size();
        std::vector<glm::mat4> skinnedTransformations(nodeCount);

        for (int i = 0; i < skinnedModel->m_nodes.size(); i++) {
            glm::mat4 nodeTransformation = glm::mat4(1);
            std::string& nodeName = skinnedModel->m_nodes[i].name;
            nodeTransformation = skinnedModel->m_nodes[i].inverseBindTransform;
            unsigned int parentIndex = skinnedModel->m_nodes[i].parentIndex;
            glm::mat4 ParentTransformation = (parentIndex == -1) ? glm::mat4(1) : skinnedTransformations[parentIndex];
            glm::mat4 GlobalTransformation = ParentTransformation * nodeTransformation;
            skinnedTransformations[i] = AnimatedTransform(GlobalTransformation).to_mat4();

            float scale = 0.01f;
            glm::vec3 position = skinnedTransformations[i][3] * scale;

            if (nodeName == "BN_Head_00") {
                m_spinePositions[0] = position;
                m_spineBoneNames[0] = nodeName;
            }
            else if (nodeName == "BN_Neck_01") {
                m_spinePositions[1] = position;
                m_spineBoneNames[1] = nodeName;
            }
            else if (nodeName == "BN_Neck_00") {
                m_spinePositions[2] = position;
                m_spineBoneNames[2] = nodeName;
            }
            else if (nodeName == "Spine_00") {
                m_spinePositions[3] = position;
                m_spineBoneNames[3] = nodeName;
            }
            else if (nodeName == "BN_Spine_01") {
                m_spinePositions[4] = position;
                m_spineBoneNames[4] = nodeName;
            }
            else if (nodeName == "BN_Spine_02") {
                m_spinePositions[5] = position;
                m_spineBoneNames[5] = nodeName;
            }
            else if (nodeName == "BN_Spine_03") {
                m_spinePositions[6] = position;
                m_spineBoneNames[6] = nodeName;
            }
            else if (nodeName == "BN_Spine_04") {
                m_spinePositions[7] = position;
                m_spineBoneNames[7] = nodeName;
            }
            else if (nodeName == "BN_Spine_05") {
                m_spinePositions[8] = position;
                m_spineBoneNames[8] = nodeName;
            }
            else if (nodeName == "BN_Spine_06") {
                m_spinePositions[9] = position;
                m_spineBoneNames[9] = nodeName;
            }
            else if (nodeName == "BN_Spine_07") {
                m_spinePositions[10] = position;
                m_spineBoneNames[10] = nodeName;
            }
        }

        // Compute distances between spine segments
        m_spinePositions[0].y = 0.0f;

        // Reset height
        for (int i = 1; i < SHARK_SPINE_SEGMENT_COUNT; i++) {
            m_spinePositions[i].y = m_spinePositions[0].y;
        }
        // Print names
        for (int i = 0; i < SHARK_SPINE_SEGMENT_COUNT; i++) {
            //std::cout << i << ": " << m_spineBoneNames[i] << "\n";
        }
        // Calculate distances
        for (int i = 0; i < SHARK_SPINE_SEGMENT_COUNT - 1; i++) {
            m_spineSegmentLengths[i] = glm::distance(m_spinePositions[i], m_spinePositions[i + 1]);
        }

        m_forward = glm::normalize(m_spinePositions[0] - m_spinePositions[1]);


        // Hack in a path
        glm::vec3 center(13.0f, 24.85, 36.0f);
        float radius = 10;
        int segments = 9;
        m_path = GetCirclePoints(center, segments, radius);
        SetPosition(center);


        m_alive = true;

    }

    static inline glm::vec3 CatmullRomUniform(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t) {
        float t2 = t * t;
        float t3 = t2 * t;
        return 0.5f * ((2.0f * p1) +
                       (-p0 + p2) * t +
                       (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                       (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
    }

    std::vector<glm::vec3> SmoothPath(std::vector<glm::vec3>& path, float spacing) {
        std::vector<glm::vec3> out;
        const int n = (int)path.size();
        if (n == 0) return out;
        if (n == 1) { out.push_back(path[0]); return out; }
        if (!(spacing > 0.0f)) return path;

        auto SafeDist = [](const glm::vec3& a, const glm::vec3& b) -> float {
            return glm::length(b - a);
        };

        std::vector<glm::vec3> dense;
        dense.reserve(n * 32);

        const float desiredStep = std::max(0.001f, spacing * 0.25f);
        for (int i = 0; i < n; ++i) {
            const glm::vec3& p0 = path[(i - 1 + n) % n];
            const glm::vec3& p1 = path[i];
            const glm::vec3& p2 = path[(i + 1) % n];
            const glm::vec3& p3 = path[(i + 2) % n];

            float chord = SafeDist(p1, p2);
            int steps = (int)std::ceil(std::max(2.0f, chord / desiredStep));
            steps = std::clamp(steps, 2, 128);

            if (i == 0) dense.push_back(p1);
            for (int s = 1; s <= steps; ++s) {
                float t = (float)s / (float)steps;
                glm::vec3 pt = CatmullRomUniform(p0, p1, p2, p3, t);
                dense.push_back(pt);
            }
        }

        {
            std::vector<glm::vec3> cleaned;
            cleaned.reserve(dense.size());
            cleaned.push_back(dense[0]);
            for (size_t i = 1; i < dense.size(); ++i) {
                if (SafeDist(cleaned.back(), dense[i]) > 1e-6f) cleaned.push_back(dense[i]);
            }
            dense.swap(cleaned);
        }

        const int m = (int)dense.size();
        if (m < 2) return dense;

        float totalLen = 0.0f;
        for (int i = 0; i < m - 1; ++i) totalLen += SafeDist(dense[i], dense[i + 1]);
        totalLen += SafeDist(dense[m - 1], dense[0]);
        if (!(totalLen > 1e-6f)) return dense;

        float ratio = totalLen / spacing;
        int outCount = (int)std::ceil(ratio - 1e-5f);
        outCount = std::max(outCount, 3);

        out.reserve(outCount);

        int segIndex = 0;
        glm::vec3 a = dense[0];
        glm::vec3 b = dense[1];
        float segLen = SafeDist(a, b);
        float travelled = 0.0f;

        auto AdvanceSegment = [&]() {
            ++segIndex;
            if (segIndex < m - 1) {
                a = dense[segIndex];
                b = dense[segIndex + 1];
            }
            else {
                a = dense[m - 1];
                b = dense[0];
            }
            segLen = SafeDist(a, b);
        };

        for (int k = 0; k < outCount; ++k) {
            float nextTarget = (float)k * spacing;

            while (travelled + segLen < nextTarget) {
                travelled += segLen;
                AdvanceSegment();

                if (segLen <= 1e-8f) {
                    int guard = 0;
                    while (segLen <= 1e-8f && guard++ < m + 4) {
                        travelled += segLen;
                        AdvanceSegment();
                    }
                    if (segLen <= 1e-8f) break;
                }
            }

            float remain = nextTarget - travelled;
            float t = (segLen > 1e-8f) ? (remain / segLen) : 0.0f;
            t = std::clamp(t, 0.0f, 1.0f);
            out.push_back(a + (b - a) * t);
        }

        return out;
    }


    void Shark::DrawDebug() {
        for (const glm::vec3& point : m_path) {
            DebugDraw::DrawPoint(point, RED);
        }

        const glm::vec3& p1 = m_spinePositions[0];
        DebugDraw::DrawPoint(p1, YELLOW);

        // Forward vector
        glm::vec3 p2 = p1 + m_forward;
        DebugDraw::DrawLine(p1, p2, YELLOW);
        DebugDraw::DrawPoint(p2, YELLOW);

        // Dir to target
        glm::vec3 a = p1 * glm::vec3(1.0f, 0.0f, 1.0f);
        glm::vec3 b = m_targetPosition * glm::vec3(1.0f, 0.0f, 1.0f);
        glm::vec3 dirToTarget = glm::normalize(b - a);
        glm::vec3 p3 = p1 + dirToTarget;;
        DebugDraw::DrawLine(p1, p3, GREEN);
        DebugDraw::DrawPoint(p3, GREEN);

        // Target XZ
        glm::vec3 p4 = m_targetPosition;
        p4.y = m_path[0].y;// p1.y;
        DebugDraw::DrawPoint(p4, WHITE);
    }

    void Shark::Update(float deltaTime) {

        // Draw path
        //for (const glm::vec3& point : m_path) {
        //    glm::vec3 p = glm::vec3(point.x, Ocean::GetOceanOriginY(), point.z);
        //    DebugDraw::DrawPoint(p, RED);
        //}

        //if (Input::KeyPressed(HELL_KEY_PERIOD)) {
        //    float spacing = 1.0f;
        //    m_path = SmoothPath(m_path, spacing);
        //}

        //if (Input::KeyPressed(HELL_KEY_COMMA)) {
        //    glm::vec3 center(10.0f, 30.0f, 58.0f);
        //    float radius = 10;
        //    int segments = 9;
        //    m_path = GetCirclePoints(center, segments, radius);
        //    m_path.erase(m_path.begin() + 2);
        //    m_path.erase(m_path.begin() + 2);
        //    m_path.erase(m_path.begin() + 2);
        //}

        // Did the player enter the water again while the shark is still angry from being like shot before
        if (m_movementState == SharkMovementState::FOLLOWING_PATH_ANGRY) {
            Unloved::Player* player = Unloved::Session::GetPlayerById(m_huntedPlayerId);
            if (player && player->FeetBelowWater()) {
                // TO DO: Only activate hunt state again if she shark has line of sight to the player
                m_movementState = SharkMovementState::HUNT_PLAYER;
                m_huntingState = SharkHuntingState::CHARGE_PLAYER;
            }
        }

        //if (Input::KeyPressed(HELL_KEY_SLASH)) {
        //    if (m_movementState == SharkMovementState::FOLLOWING_PATH) {
        //        m_movementState = SharkMovementState::ARROW_KEYS;
        //    }
        //    else {
        //        m_movementState = SharkMovementState::FOLLOWING_PATH;
        //        m_nextPathPointIndex = 0;
        //    }
        //}

        // Put these somewhere better!
        m_right = glm::cross(m_forward, glm::vec3(0, 1, 0));
        m_left = -m_right;

        glm::mat4 rootTranslationMatrix = glm::translate(glm::mat4(1), m_spinePositions[3]);

        // Root to the end of the spine
        float rot0 = Hell::Math::YawBetweenPoints(m_spinePositions[3], m_spinePositions[2]) + HELL_PI * 0.5f;
        float rot1 = Hell::Math::YawBetweenPoints(m_spinePositions[4], m_spinePositions[3]) + HELL_PI * 0.5f;
        float rot2 = Hell::Math::YawBetweenPoints(m_spinePositions[5], m_spinePositions[4]) + HELL_PI * 0.5f;
        float rot3 = Hell::Math::YawBetweenPoints(m_spinePositions[6], m_spinePositions[5]) + HELL_PI * 0.5f;
        float rot4 = Hell::Math::YawBetweenPoints(m_spinePositions[7], m_spinePositions[6]) + HELL_PI * 0.5f;
        float rot5 = Hell::Math::YawBetweenPoints(m_spinePositions[8], m_spinePositions[7]) + HELL_PI * 0.5f;
        float rot6 = Hell::Math::YawBetweenPoints(m_spinePositions[9], m_spinePositions[8]) + HELL_PI * 0.5f;
        float rot7 = Hell::Math::YawBetweenPoints(m_spinePositions[10], m_spinePositions[9]) + HELL_PI * 0.5f;

        // From the neck to the head
        float rot8 = Hell::Math::YawBetweenPoints(m_spinePositions[3], m_spinePositions[2]) + HELL_PI * 0.5f;
        float rot9 = Hell::Math::YawBetweenPoints(m_spinePositions[2], m_spinePositions[1]) + HELL_PI * 0.5f;
        float rot10 = Hell::Math::YawBetweenPoints(m_spinePositions[1], m_spinePositions[0]) + HELL_PI * 0.5f;

        // Update animation
        Unloved::AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
        if (animatedGameObject) {
            glm::vec3 rootPosition = m_spinePositions[3];
            float yRotation = rot0;

            if (IsAlive()) {
                float magicNumber = 1.05f;
                float lerpSpeed = 1.0f;

                // Start with the target at the ocean surface
                float targetHeight = Ocean::GetOceanOriginY() - magicNumber;

                // But if hunting a player overwrite it with their height
                if (m_movementState == SharkMovementState::HUNT_PLAYER) {
                    if (Unloved::Player* player = Unloved::Session::GetPlayerById(m_huntedPlayerId)) {
                        targetHeight = std::min(targetHeight, player->GetCameraPosition().y) - (magicNumber * 0.0f);
                    }
                }

                // Lerp to target
                m_yHeight = Hell::Math::InterpTo(m_yHeight, targetHeight, deltaTime, lerpSpeed);

                // Calculate new position. Which is the spine pos plus the hacked in y height
                glm::vec3 position = rootPosition;
                position.y = m_yHeight;

                // Set it
                animatedGameObject->SetPosition(position);
            }

            animatedGameObject->SetRotationY(yRotation);

            animatedGameObject->SetAdditiveTransform("BN_Spine_01", glm::rotate(glm::mat4(1.0f), rot1 - rot0, glm::vec3(0, 1, 0)));
            animatedGameObject->SetAdditiveTransform("BN_Spine_02", glm::rotate(glm::mat4(1.0f), rot2 - rot1, glm::vec3(0, 1, 0)));
            animatedGameObject->SetAdditiveTransform("BN_Spine_03", glm::rotate(glm::mat4(1.0f), rot3 - rot2, glm::vec3(0, 1, 0)));
            animatedGameObject->SetAdditiveTransform("BN_Spine_04", glm::rotate(glm::mat4(1.0f), rot4 - rot3, glm::vec3(0, 1, 0)));
            animatedGameObject->SetAdditiveTransform("BN_Spine_05", glm::rotate(glm::mat4(1.0f), rot5 - rot4, glm::vec3(0, 1, 0)));
            animatedGameObject->SetAdditiveTransform("BN_Spine_06", glm::rotate(glm::mat4(1.0f), rot6 - rot5, glm::vec3(0, 1, 0)));
            animatedGameObject->SetAdditiveTransform("BN_Spine_07", glm::rotate(glm::mat4(1.0f), rot7 - rot6, glm::vec3(0, 1, 0)));
            animatedGameObject->SetAdditiveTransform("BN_Neck_00", glm::rotate(glm::mat4(1.0f), rot8 - rot1, glm::vec3(0, 1, 0)));
            animatedGameObject->SetAdditiveTransform("BN_Neck_01", glm::rotate(glm::mat4(1.0f), rot9 - rot8, glm::vec3(0, 1, 0)));
            animatedGameObject->SetAdditiveTransform("BN_Head_00", glm::rotate(glm::mat4(1.0f), rot10 - rot9, glm::vec3(0, 1, 0)));

            animatedGameObject->Update(deltaTime);
            animatedGameObject->UpdateRenderItems();

            if (Ragdoll* ragdoll = GetRagdoll()) {
                if (Renderer::GetCurrentRendererSettings().debugDrawRagdolls) {
                    animatedGameObject->DisableRendering();
                }
                else {
                    animatedGameObject->EnableRendering();
                }
            }
        }

        // Arrow key movement
        if (IsAlive()) {
            if (m_movementState == SharkMovementState::ARROW_KEYS) {
                if (Input::KeyDown(HELL_KEY_UP)) {
                    CalculateForwardVectorFromArrowKeys(deltaTime);

                    for (int i = 0; i < m_logicSubStepCount; i++) {
                        MoveShark(deltaTime);
                    }
                }
            }
            // Path following
            else if (m_movementState == SharkMovementState::FOLLOWING_PATH ||
                     m_movementState == SharkMovementState::FOLLOWING_PATH_ANGRY) {
                CalculateTargetFromPath();
                CalculateForwardVectorFromTarget(deltaTime);

                for (int i = 0; i < m_logicSubStepCount; i++) {
                    MoveShark(deltaTime);
                }
            }
            else if (m_movementState == SharkMovementState::HUNT_PLAYER) {
                if (m_huntingState == SharkHuntingState::CHARGE_PLAYER ||
                    m_huntingState == SharkHuntingState::BITING_PLAYER && GetAnimationFrameNumber() > 17 ||
                    m_huntingState == SharkHuntingState::BITING_PLAYER && GetAnimationFrameNumber() > 7 && !IsBehindEvadePoint(m_targetPosition)) {
                    CalculateTargetFromPlayer();
                    CalculateForwardVectorFromTarget(deltaTime);
                }
                for (int i = 0; i < m_logicSubStepCount; i++) {
                    UpdateHuntingLogic(deltaTime);
                    MoveShark(deltaTime);
                }
            }
        }

        // Is it alive
        if (m_health > 0) {
            m_alive = true;
        }
        // Kill if health zero
        if (IsAlive() && m_health <= 0) {
            Kill();
            //Game::g_sharkDeaths++;
            //std::ofstream out("SharkDeaths.txt");
            //out << Game::g_sharkDeaths;
            //out.close();
        }
        m_health = std::max(m_health, 0);

        //if (IsDead()) {
        //    StraightenSpine(deltaTime, 0.25f);
        //    if (animatedGameObject->GetAnimationFrameNumber("MainLayer") > 100) {
        //        animatedGameObject->PauseAllAnimationLayers();
        //    }
        //}

        if (IsDead()) {
            StraightenSpine(deltaTime, 0.25f);

            // Sink instantly on death
            const glm::vec3& currentPosition = animatedGameObject->GetPosition();
            float sinkAmount = deltaTime * 1.36f;
            glm::vec3 newPosition = currentPosition + glm::vec3(0.0f, -sinkAmount, 0.0f);
            animatedGameObject->SetPosition(newPosition);

            //std::cout << "currentPosition: " << currentPosition << " " << "newPosition: " << newPosition << "\n";

            // PAUSE IT AT FRAME 100 of death anim
            if (animatedGameObject->IsAllAnimationsComplete()) {
                animatedGameObject->PauseAllAnimationLayers();
            }

            // RESPAWN AFTER FALLING FAR ENOUGH
            if (currentPosition.y < 10.0f) {
                Respawn();
                float spawnHeight = 28.85f;
                glm::vec3 spawnPos = glm::vec3(-50.0f, spawnHeight, 40.5f);
                animatedGameObject->SetPosition(spawnPos); // is this necessary?
                SetPosition(spawnPos);
            }
        }


        // Find closest player
       //float distanceToClosestPlayer = 99999;
       //float safeHeight = Ocean::GetOceanOriginY() - 1.19f;
       //
       //bool targetFound = false;
       //
       //// If shark ain't biting, then find next closest target if any
       //if (m_huntingState != SharkHuntingState::BITING_PLAYER && m_movementState == SharkMovementState::FOLLOWING_PATH) {
       //    for (int i = 0; i < Game::GetLocalPlayerCount(); i++) {
       //        Unloved::Player* player = Game::GetLocalPlayerByViewportIndex(i);
       //        if (!player) continue;
       //
       //        Unloved::AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
       //        float distanceToPlayer = glm::distance(animatedGameObject->GetPosition(), player->GetFootPosition());
       //
       //        if (player->GetFootPosition().y < safeHeight && distanceToPlayer < distanceToClosestPlayer) {
       //            distanceToClosestPlayer = distanceToPlayer;
       //            HuntPlayer(player->GetPlayerId());
       //            targetFound = true;
       //        }
       //    }
       //}
       //
       //if (!targetFound) {
       //    m_movementState = SharkMovementState::FOLLOWING_PATH;
       //}


        //DrawDebug();
    }

    void Shark::CleanUp() {
        Unloved::AnimatedGameObject* animatedGameObject = GetAnimatedGameObject();
        if (animatedGameObject) {
            animatedGameObject->CleanUp();
            Unloved::World::GetAnimatedGameObjects().erase(g_animatedGameObjectObjectId);
        }
        g_animatedGameObjectObjectId = 0;
    }

    void Shark::StraightenSpine(float deltaTime, float straightSpeed) {
        // Fake moving the head forward
        glm::vec3 originalHeadPosition = m_spinePositions[0];
        glm::vec3 fakeForwardMovement = GetForwardVector() * m_swimSpeed * deltaTime * straightSpeed;
        m_spinePositions[0] += fakeForwardMovement;
        // Straighten the rest of the spine using movement logic
        for (int i = 1; i < SHARK_SPINE_SEGMENT_COUNT; ++i) {
            glm::vec3 direction = m_spinePositions[i - 1] - m_spinePositions[i];
            float currentDistance = glm::length(direction);
            if (currentDistance > m_spineSegmentLengths[i - 1]) {
                glm::vec3 correction = glm::normalize(direction) * (currentDistance - m_spineSegmentLengths[i - 1]);
                m_spinePositions[i] += correction;
            }
        }
        // Move the head back to its original position
        glm::vec3 correctionVector = m_spinePositions[0] - originalHeadPosition;
        for (int i = 0; i < SHARK_SPINE_SEGMENT_COUNT; ++i) {
            m_spinePositions[i] -= correctionVector;
        }
    }

    void Shark::GiveDamage(uint64_t playerId, int damageAmount) {
        m_health -= damageAmount;
        std::cout << "Shark health: " << m_health << "\n";
        //if (m_huntedPlayerId == 0) {
        HuntPlayer(playerId);
        //}
    }

    void Shark::HuntPlayer(uint64_t playerId) {
        m_huntedPlayerId = playerId;
        m_movementState = SharkMovementState::HUNT_PLAYER;
        m_huntingState = SharkHuntingState::CHARGE_PLAYER;
    }

    void Shark::SetPositionToBeginningOfPath() {
        if (m_path.empty()) {
            SetPosition(glm::vec3(0, 30.0f, 0));
        }
        else {
            glm::vec3 position = m_path[0];
            SetPosition(position);
            m_nextPathPointIndex = 1;
        }
    }

    void Shark::Respawn() {
        SetPositionToBeginningOfPath();
        m_movementState = SharkMovementState::FOLLOWING_PATH;
        m_health = SHARK_HEALTH_MAX;
        m_alive = true;
        m_hasBitPlayer = false;
        if (Ragdoll* ragdoll = GetRagdoll()) {
            ragdoll->SetToInitialPose();
            ragdoll->DisableSimulation();
        }
        if (Unloved::AnimatedGameObject* animatedGameObject = GetAnimatedGameObject()) {
            animatedGameObject->SetAnimationModeToAnimated();
        }
        PlayAndLoopAnimation("Shark_Swim", 1.0f);
    }

    void Shark::Kill() {
        m_health = 0;
        m_alive = false;
        Audio::PlayAudio("Shark_Death.wav", 1.0f);
        PlayAnimation("Shark_Die", 1.0f);
    }

    void Shark::SetMovementState(SharkMovementState movementState) {
        m_movementState = movementState;
    }

    void Shark::SetPosition(const glm::vec3& position) {
        m_spinePositions[0] = position;
        for (int i = 1; i < SHARK_SPINE_SEGMENT_COUNT; i++) {
            m_spinePositions[i].x = m_spinePositions[0].x;
            m_spinePositions[i].y = m_spinePositions[0].y;
            m_spinePositions[i].z = m_spinePositions[i - 1].z - m_spineSegmentLengths[i - 1];
            //m_rotation = 0;           TRIPLE CHECK YOU DON'T NEED THIS !!!!!!!!!!!!!!!!!!!!!!!!!!
        }
        m_forward = glm::vec3(0, 0, 1);
    }

    void Shark::CalculateTargetFromPlayer() {
        if (m_huntedPlayerId != 0) {
            Unloved::Player* player = Unloved::Session::GetPlayerById(m_huntedPlayerId);
            m_targetPosition = player->GetCameraPosition() - glm::vec3(0.0, 0.1f, 0.0f);
            //std::cout << Hell::String::FormatVec3(m_targetPosition) << "\n";

            static bool attackLeft = true;
            if (GetDistanceToTarget2D() < 6.5f) {
                if (attackLeft) {
                    m_targetPosition += m_left * 0.975f;
                    //std::cout << "attacking left\n";
                }
                else {
                    m_targetPosition += m_right * 0.975f;
                    //std::cout << "attacking right\n";
                }
            }
            else {
                attackLeft = !attackLeft;
                //std::cout << "attack left: " << attackLeft << "\n";
            }
            m_lastKnownTargetPosition = m_targetPosition;
        }
    }

    // Forward-only (wrap) scan starting at startIndex. Returns an index into path.
    // Returns startIndex if nothing qualifies as "in front" (so it never go backwards).
    int GetClosestPointNotBehindSharkIndex_ForwardOnly(const std::vector<glm::vec3>& path,
                                                       int startIndex,
                                                       const glm::vec3& currentPosition,
                                                       const glm::vec3& currentForward,
                                                       float dotThreshold) {
        if (path.empty()) return 0;

        const int pathSize = (int)path.size();
        if (startIndex < 0) startIndex = 0;
        if (startIndex >= pathSize) startIndex = 0;

        glm::vec3 posXZ = currentPosition;
        posXZ.y = 0.0f;

        glm::vec3 forwardXZ = NormalizeXZOr(currentForward, glm::vec3(0.0f, 0.0f, 1.0f));

        int bestIndex = -1;
        float bestDistSq = 0.0f;

        for (int step = 0; step < pathSize; step++) {
            int i = (startIndex + step) % pathSize;

            glm::vec3 p = path[i];
            p.y = 0.0f;

            glm::vec3 toPoint = p - posXZ;
            toPoint.y = 0.0f;

            float distSq = glm::dot(toPoint, toPoint);
            if (distSq < 0.000001f) {
                return i;
            }

            float invLen = 1.0f / std::sqrt(distSq);
            glm::vec3 toDir = toPoint * invLen;

            float d = glm::dot(forwardXZ, toDir);
            if (d <= dotThreshold) continue;

            if (bestIndex == -1 || distSq < bestDistSq) {
                bestIndex = i;
                bestDistSq = distSq;
            }
        }

        if (bestIndex == -1) return startIndex;
        return bestIndex;
    }

    void Shark::CalculateTargetFromPath() {
        if (m_path.empty()) return;

        const int pathSize = (int)m_path.size();
        if (m_nextPathPointIndex >= pathSize) m_nextPathPointIndex = 0;
        if (m_nextPathPointIndex < 0) m_nextPathPointIndex = 0;

        glm::vec3 headPos = GetHeadPosition2D();
        headPos.y = 0.0f;

        glm::vec3 forwardXZ = m_forward;
        forwardXZ.y = 0.0f;

        const float inFrontDotThreshold = 0.25f;

        m_nextPathPointIndex = GetClosestPointNotBehindSharkIndex_ForwardOnly(
            m_path,
            m_nextPathPointIndex,
            headPos,
            forwardXZ,
            inFrontDotThreshold
        );

        m_targetPosition = m_path[m_nextPathPointIndex];
        m_targetPosition.y = 0.0f;
    }

    void Shark::MoveShark(float deltaTime) {

        //m_mouthPositionLastFrame = GetMouthPosition2D();
        //m_headPositionLastFrame = GetHeadPosition2D();
        //m_evadePointPositionLastFrame = GetEvadePoint2D();
        // Move head
        m_spinePositions[0] += m_forward * m_swimSpeed * deltaTime / (float)m_logicSubStepCount;
        // Move spine segments
        for (int i = 1; i < SHARK_SPINE_SEGMENT_COUNT; ++i) {
            glm::vec3 direction = glm::normalize(m_spinePositions[i - 1] - m_spinePositions[i]);
            m_spinePositions[i] = m_spinePositions[i - 1] - direction * m_spineSegmentLengths[i - 1];
        }
    }


    struct BezierSegment {
        glm::vec3 p0;
        glm::vec3 p1;
        glm::vec3 p2;
        glm::vec3 p3;
    };

    static inline glm::vec3 BezierPoint(const BezierSegment& s, float t) {
        float u = 1.0f - t;
        float tt = t * t;
        float uu = u * u;
        float uuu = uu * u;
        float ttt = tt * t;
        return (uuu * s.p0) + (3.0f * uu * t * s.p1) + (3.0f * u * tt * s.p2) + (ttt * s.p3);
    }

    static inline glm::vec3 BezierTangent(const BezierSegment& s, float t) {
        float u = 1.0f - t;
        return 3.0f * (u * u) * (s.p1 - s.p0)
            + 6.0f * (u * t) * (s.p2 - s.p1)
            + 3.0f * (t * t) * (s.p3 - s.p2);
    }

    // Builds a cubic Bezier for segment i: path[i] -> path[i+1], closed loop.
    // Tension in [0..1]. 0.5 is a solid default.
    static inline BezierSegment BuildBezierSegment_ClosedLoop(const std::vector<glm::vec3>& path, int i, float tension) {
        const int n = (int)path.size();
        int i0 = (i - 1 + n) % n;
        int i1 = (i + 0) % n;
        int i2 = (i + 1) % n;
        int i3 = (i + 2) % n;

        glm::vec3 p0 = path[i1];
        glm::vec3 p3 = path[i2];

        glm::vec3 t0 = (path[i2] - path[i0]) * (tension * 0.5f);
        glm::vec3 t1 = (path[i3] - path[i1]) * (tension * 0.5f);

        BezierSegment s;
        s.p0 = p0;
        s.p3 = p3;

        // Convert Hermite-style endpoints+tangents to Bezier controls.
        s.p1 = p0 + t0 / 3.0f;
        s.p2 = p3 - t1 / 3.0f;
        return s;
    }

    // Cheap length approximation (good enough for stepping t by distance).
    static inline float ApproxBezierLength(const BezierSegment& s) {
        const int samples = 10;
        float len = 0.0f;
        glm::vec3 prev = BezierPoint(s, 0.0f);
        for (int k = 1; k <= samples; k++) {
            float t = (float)k / (float)samples;
            glm::vec3 p = BezierPoint(s, t);
            len += glm::length(p - prev);
            prev = p;
        }
        return len;
    }


    void Shark::CalculateForwardVectorFromTarget(float deltaTime) {
        // extract current orientations
        glm::vec3 headPos = GetHeadPosition2D();
        glm::vec3 targetPos = GetTargetPosition2D();

        // compute desired forward
        glm::vec3 forwardXZ = NormalizeXZOr(m_forward, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::vec3 desiredXZ = NormalizeXZOr(targetPos - headPos, forwardXZ);

        // find angle delta
        float dotValue = glm::clamp(glm::dot(forwardXZ, desiredXZ), -1.0f, 1.0f);
        float crossY = forwardXZ.x * desiredXZ.z - forwardXZ.z * desiredXZ.x;
        float signedAngle = std::atan2(crossY, dotValue);

        // scale turn rate to ease in as shark aligns
        float baseTurnRate = glm::radians(225.0f);
        float alignmentScale = glm::clamp(std::abs(signedAngle) / glm::radians(45.0f), 0.1f, 1.0f);
        float currentTurnRate = baseTurnRate * alignmentScale;

        // clamp angle delta
        float maxStep = currentTurnRate * deltaTime;
        if (signedAngle > maxStep) signedAngle = maxStep;
        if (signedAngle < -maxStep) signedAngle = -maxStep;

        // build new forward vector
        float c = std::cos(signedAngle);
        float s = std::sin(signedAngle);

        glm::vec3 newForward;
        newForward.x = forwardXZ.x * c - forwardXZ.z * s;
        newForward.z = forwardXZ.x * s + forwardXZ.z * c;
        newForward.y = 0.0f;
        m_forward = NormalizeXZOr(newForward, forwardXZ);

        // update orthogonal vectors
        const glm::vec3 up(0.0f, 1.0f, 0.0f);
        m_right = NormalizeXZOr(glm::cross(up, m_forward), glm::vec3(1.0f, 0.0f, 0.0f));
        m_left = -m_right;
    }

    void Shark::CalculateForwardVectorFromArrowKeys(float deltaTime) {
        float maxRotation = 5.0f;
        if (Input::KeyDown(HELL_KEY_LEFT)) {
            float blendFactor = glm::clamp(glm::abs(-maxRotation) / 90.0f, 0.0f, 1.0f);
            m_forward = glm::normalize(glm::mix(m_forward, m_left, blendFactor));
            std::cout << "PRESSED LEFT\n";
        }
        if (Input::KeyDown(HELL_KEY_RIGHT)) {
            float blendFactor = glm::clamp(glm::abs(maxRotation) / 90.0f, 0.0f, 1.0f);
            m_forward = glm::normalize(glm::mix(m_forward, m_right, blendFactor));
            std::cout << "PRESSED RIGHT\n";
        }
    }

    glm::vec3 Shark::GetForwardVector() {
        return m_forward;
    }

    glm::vec3 Shark::GetTargetPosition2D() {
        return m_targetPosition * glm::vec3(1.0f, 0.0f, 1.0f);
    }

    glm::vec3 Shark::GetHeadPosition2D() {
        return m_spinePositions[0] * glm::vec3(1.0f, 0.0f, 1.0f);
    }

    glm::vec3 Shark::GetCollisionLineEnd() {
        return GetCollisionSphereFrontPosition() + (GetForwardVector() * GetTurningRadius());
    }

    glm::vec3 Shark::GetCollisionSphereFrontPosition() {
        return GetHeadPosition2D() + GetForwardVector() * glm::vec3(COLLISION_SPHERE_RADIUS);
    }

    float Shark::GetTurningRadius() const {
        float turningRadius = m_swimSpeed / m_rotationSpeed;
        return turningRadius;
    }

    bool Shark::TargetIsOnLeft(glm::vec3 targetPosition) {
        glm::vec3 lineStart = GetHeadPosition2D();
        glm::vec3 lineEnd = GetCollisionLineEnd();
        glm::vec3 lineNormal = Hell::LineMath::GetLineNormal(lineStart, lineEnd);
        glm::vec3 midPoint = Hell::LineMath::GetLineMidPoint(lineStart, lineEnd);
        return Hell::LineMath::IsPointOnOtherSideOfLine(lineStart, lineEnd, lineNormal, targetPosition);
    }

    float Shark::GetDistanceToTarget2D() {
        return glm::distance(GetHeadPosition2D() * glm::vec3(1, 0, 1), m_targetPosition * glm::vec3(1, 0, 1));
    }

    /*
    void Shark::CalculateForwardVectorFromTarget(float deltaTime) {
        // Calculate angular difference from forward to target
        glm::vec3 directionToTarget = glm::normalize(GetTargetPosition2D() - GetHeadPosition2D());
        float dotProduct = glm::clamp(glm::dot(m_forward, directionToTarget), -1.0f, 1.0f);
        float angleDifference = glm::degrees(std::acos(dotProduct));
        if (m_forward.x * directionToTarget.z - m_forward.z * directionToTarget.x < 0.0f) {
            angleDifference = -angleDifference;
        }
        // Clamp it to a max of 4.5 degrees rotation
        float maxRotation = 4.5f;
        angleDifference = glm::clamp(angleDifference, -maxRotation, maxRotation);
        // Calculate new forward vector based on that angle
        if (TargetIsOnLeft(m_targetPosition)) {
            float blendFactor = glm::clamp(glm::abs(-angleDifference) / 90.0f, 0.0f, 1.0f);
            m_forward = glm::normalize(glm::mix(m_forward, m_left, blendFactor));
        }
        else {
            float blendFactor = glm::clamp(glm::abs(angleDifference) / 90.0f, 0.0f, 1.0f);
            m_forward = glm::normalize(glm::mix(m_forward, m_right, blendFactor));
        }
    }

    bool Shark::TargetIsOnLeft(glm::vec3 targetPosition) {
        glm::vec3 lineStart = GetHeadPosition2D();
        glm::vec3 lineEnd = GetCollisionLineEnd();
        glm::vec3 lineNormal = Hell::LineMath::GetLineNormal(lineStart, lineEnd);
        glm::vec3 midPoint = Hell::LineMath::GetLineMidPoint(lineStart, lineEnd);
        return Hell::LineMath::IsPointOnOtherSideOfLine(lineStart, lineEnd, lineNormal, targetPosition);
    }
    */

    Unloved::AnimatedGameObject* Shark::GetAnimatedGameObject() {
        return Unloved::World::GetAnimatedGameObjectByObjectId(g_animatedGameObjectObjectId);
    }

    void Shark::DrawSpinePoints() {
        for (int i = 1; i < SHARK_SPINE_SEGMENT_COUNT; ++i) {
            DebugDraw::DrawPoint(m_spinePositions[i], RED);
        }
        for (int i = 0; i < 1; ++i) {
            DebugDraw::DrawPoint(m_spinePositions[i], WHITE);
        }
    }

    Ragdoll* Shark::GetRagdoll() {
        if (m_RagdollId == 0) {
            return nullptr;
        }
        return Hell::Physics::GetRagdollById(m_RagdollId);
    }
}
