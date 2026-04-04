#include <srl.hpp>

// Using to shorten names for Vector and HighColor
using namespace SRL::Types;
using namespace SRL::Math::Types;
using namespace SRL::Input;

const auto DarkGreen = HighColor::FromRGB555(0, 8, 0);

/** @brief Volume meter circle object
 */
class VolumeMeterCircle
{
private:
    /** @brief Angular step
     */
    inline static const auto step = Angle::FromDegrees(20);

    /** @brief Base circle Z position
     */
    Fxp baseDepth;

    /** @brief Base circle location
     */
    Vector3D baseLocation;

    /** @brief Base circle radius
     */
    Fxp baseRadius;

    /** @brief Circle mesh
     */
    Mesh* mesh;

    /** @brief Last value meter has responded to
     */
    uint16_t lastResponse;

    /** @brief Generate volume meter circle mesh
     */
    void GenerateVolumeMeterCircle()
    {
        const size_t vertices = (Fxp(360.0) / step.ToDegrees()).As<uint16_t>();
        this->mesh = new Mesh(vertices, vertices);

        // Create circle
        Angle angle = Angle::Zero();
        size_t index = 0;

        for (; index < vertices; angle += step, index++)
        {
            const auto sin = SRL::Math::Trigonometry::Sin(angle);
            const auto cos = SRL::Math::Trigonometry::Cos(angle);

            // Set face vertices to current and next
            auto face = &this->mesh->Faces[index];
            face->Normal = Vector3D(0.0, 0.0, 1.0);
            face->Vertices[0] = index;
            face->Vertices[1] = (index + 1) % vertices;
            face->Vertices[2] = face->Vertices[0];
            face->Vertices[3] = face->Vertices[1];

            // Set quad type to line and color to dark green
            this->mesh->Attributes[index] = Attribute(
                Attribute::FaceVisibility::DoubleSided,
                Attribute::SortMode::Center,
                No_Texture,
                DarkGreen,
                CL32KRGB,
                CL32KRGB,
                sprLine,
                No_Option);

            // Create 3D point in X/Z
            const auto point = Vector3D(this->baseRadius * cos, this->baseDepth, this->baseRadius * sin) + this->baseLocation;
            this->mesh->Vertices[index] = point;
        }
    }

    /** @brief Remove default constructor
     */
    VolumeMeterCircle() = delete;

public:

    /** @brief Initialize volume meter circle instance
     * @param location Circle base location
     * @param radius Circle radius
     * @param depth Circle Z position
     */
    VolumeMeterCircle(const Vector3D& location, const Fxp radius, const Fxp depth) : baseRadius(radius), baseDepth(depth), baseLocation(location), lastResponse(0)
    {
        this->GenerateVolumeMeterCircle();
    }

    /** @brief Destroy the Volume Meter Circle object
     */
    ~VolumeMeterCircle()
    {
        delete this->mesh;
    }

    /** @brief Draw the circle
     */
    void Draw()
    {
        SRL::Scene3D::DrawMesh(*this->mesh);
    }

    /** @brief Set volume to respond to
     * @param volume Volume level
     */
    void SetResponse(const uint16_t volume)
    {
        lastResponse = volume;
        const auto newRadius = this->baseRadius + Fxp::BuildRaw(volume << 5);
        auto angle = Angle::Zero();

        // Map volume to color addition
        const static auto scale = 24.0 / Fxp::BuildRaw(14800 << 5);
        const auto colorOffset = (uint16_t)std::min((scale * Fxp::BuildRaw(volume << 5)).As<uint16_t>() << 2, 23);

        for (size_t index = 0; index < this->mesh->VertexCount;  angle += step, index++)
        {
            const auto sin = SRL::Math::Trigonometry::Sin(angle);
            const auto cos = SRL::Math::Trigonometry::Cos(angle);

            // Update 3D point in X/Z
            const auto point = Vector3D(
                this->baseLocation.X + (newRadius * cos),
                this->mesh->Vertices[index].Y,
                this->baseLocation.Z + (newRadius * sin));

            this->mesh->Vertices[index] = point;

            // Update face color
            this->mesh->Attributes[index].ColorMode = HighColor::FromRGB555(DarkGreen.Red, DarkGreen.Green + colorOffset, DarkGreen.Blue);
        }
    }

    /** @brief Get the last volume meter has responded to
     * @return uint16_t Responded volume level
     */
    uint16_t GetResponse() 
    {
        return this->lastResponse;
    }
};

class VolumeMeter
{
private:
    /** @brief Number of volume meter circles
     */
    const size_t circleCount;

    /** @brief Collection of volume meter circles
     */
    VolumeMeterCircle** circles;

    /** @brief Remove default constructor
     */
    VolumeMeter() = delete;

public:

    /** @brief Construct a new Volume Meter object
     * @param location Meter location
     * @param radius Circle radius
     * @param depth Circle Z position increment
     * @param circles Number of meter circles to generate
     */
    VolumeMeter(const Vector3D& location, const Fxp radius, const Fxp depthStep, const size_t circles) : circleCount(circles)
    {
        this->circles = new VolumeMeterCircle*[circles];
        Fxp depth = 0.0;

        for (size_t circle = 0; circle < circles; circle++, depth += depthStep)
        {
            this->circles[circle] = new VolumeMeterCircle(location, radius, depth);
        }
    }

    /** @brief Destroy the Volume Meter object
     */
    ~VolumeMeter()
    {
        for (size_t circle = 0; circle < this->circleCount; circle++)
        {
            delete this->circles[circle];
        }

        delete[] this->circles;
    }

    /** @brief Push volume into the meter history
     */
    void PushVolume(const uint16_t volume)
    {
        uint16_t lastVolume = volume;

        for (size_t circle = 0; circle < this->circleCount; circle++)
        {
            uint16_t currentVolume = this->circles[circle]->GetResponse();
            this->circles[circle]->SetResponse(lastVolume);
            lastVolume = currentVolume;
        }
    }

    /** @brief Draw volume meter
     */
    void Draw()
    {
        for (size_t circle = 0; circle < this->circleCount; circle++)
        {
            this->circles[circle]->Draw();
        }
    }
};

enum class PlaybackStatus {
    STOPPED,
    PLAYING,
    PAUSED
};

// Main program entry 
int main()
{
    // Initialize library
    SRL::Core::Initialize(HighColor::Colors::Black);

    Digital port0(0); // Initialize gamepad on port 0

    SRL::Debug::Print(1,  1, "Sound CDDA sample");
    SRL::Debug::Print(1, 24, "<Y [Stop]>");
    SRL::Debug::Print(1, 25, "<Z [Play/Pause]>");
    SRL::Debug::Print(1, 26, "<L [Prev] R [Next]>");
    
    // Setup camera location
    Vector3D cameraLocation = Vector3D(0.0, -20.0, -40.0);
    
    // Load Volume meter
    auto leftMeter = new VolumeMeter(Vector3D(10.0, 0.0, 0.0), 3.0, -1.0, 16);
    auto rightMeter = new VolumeMeter(Vector3D(-10.0, 0.0, 0.0), 3.0, -1.0, 16);
    
    // Initialize rotation angle
    Angle rotation = 0;
    Angle rotationStep = Angle::FromDegrees(0.2);
    
    // Start analyzers
    SRL::Sound::Cdda::Analysis::Start();
    
    // CD playback state
    size_t currentTrack = 2;
    const size_t numberOfTracks = 4; // including the data track
    const bool repeat = true; // Always repeat tracks
    PlaybackStatus status = PlaybackStatus::PLAYING;

    // Play first CD audio track on loop
    SRL::Sound::Cdda::PlaySingle(currentTrack, repeat);
    
    const size_t trackTitleBufferLen = 10;
    char trackTitleBuffer[trackTitleBufferLen];

    // Main loop
    size_t updateTick = 0;

	while(1)
	{
        // Handle input
        if (port0.IsConnected())
        {
            if (port0.WasPressed(Digital::Button::R))
            {
                currentTrack = (currentTrack == numberOfTracks) ? 2 : currentTrack + 1;
                SRL::Sound::Cdda::PlaySingle(currentTrack, repeat);
                status = PlaybackStatus::PLAYING;
            }
            else if (port0.WasPressed(Digital::Button::L))
            {
                currentTrack = (currentTrack == 2) ? numberOfTracks : currentTrack - 1;
                SRL::Sound::Cdda::PlaySingle(currentTrack, repeat);
                status = PlaybackStatus::PLAYING;
            }
            else if (port0.WasPressed(Digital::Button::Y))
            {
                SRL::Sound::Cdda::StopPause();
                status = PlaybackStatus::STOPPED;
            }
            else if (port0.WasPressed(Digital::Button::Z))
            {
                if (status == PlaybackStatus::PLAYING)
                {
                    SRL::Sound::Cdda::StopPause();
                    status = PlaybackStatus::PAUSED;
                }
                else
                {
                    if (status == PlaybackStatus::STOPPED)
                    {
                        SRL::Sound::Cdda::PlaySingle(currentTrack, repeat);
                    }
                    else // PAUSED
                    {
                        SRL::Sound::Cdda::Resume();
                    }
                    status = PlaybackStatus::PLAYING;
                }
            }
        }

        // Update UI
        snprintf(trackTitleBuffer, trackTitleBufferLen, "Track %02u", currentTrack);
        SRL::Debug::Print(1, 23, trackTitleBuffer);
        
        // Display playback status
        switch (status)
        {
            case PlaybackStatus::PLAYING:
                SRL::Debug::Print(1, 22, "Status: PLAYING");
                break;
            case PlaybackStatus::PAUSED:
                SRL::Debug::Print(1, 22, "Status: PAUSED ");
                break;
            case PlaybackStatus::STOPPED:
                SRL::Debug::Print(1, 22, "Status: STOPPED");
                break;
        }

        // Load identity matrix
        SRL::Scene3D::LoadIdentity();
        
        // Set camera location and direction
        SRL::Scene3D::LookAt(cameraLocation, Vector3D(), Angle::Zero());

        // Rotate meters
        SRL::Scene3D::RotateY(rotation);
        rotation += rotationStep;

        // Get volume analysis only at slower speed to make it look nice
        if (updateTick++ > 1)
        {
            updateTick = 0;

            // For the track in this sample maximal volume value is around 14.7k
            const auto volume = SRL::Sound::Cdda::Analysis::GetTotalVolume();
            leftMeter->PushVolume(volume.LeftChannel);
            rightMeter->PushVolume(volume.RightChannel);
        }

        leftMeter->Draw();
        rightMeter->Draw();

        // Refresh screen
        SRL::Core::Synchronize();
	}

	return 0;
}
