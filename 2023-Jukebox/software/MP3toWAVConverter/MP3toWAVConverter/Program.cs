using ClosedXML.Excel;
using MP3toWAVConverter;
using System.Diagnostics;

const int ID_COL = 1;
const int SONG_NAME_COL = 2;
const int ARTIST_COL = 3;
const int YEAR_COL = 4;
const int SELECTED_COL = 5;

if (Directory.Exists("Output"))
    Directory.Delete("Output", true);
Directory.CreateDirectory("Output");

List<SongInfo> songs = new();

using (var excelWorkbook = new XLWorkbook("songs.xlsx"))
{
    foreach (var dataRow in excelWorkbook.Worksheet(1).RowsUsed().Skip(2))
    {
        if (dataRow.Cell(SELECTED_COL).Value.ToString().Trim().ToLower() == "no")
            continue;

        if (!dataRow.Cell(ID_COL).IsEmpty() && dataRow.Cell(ID_COL).Value.ToString().Trim().Length > 0)
            songs.Add(new SongInfo(dataRow.Cell(ID_COL).Value.ToString(), 
                                   dataRow.Cell(SONG_NAME_COL).Value.ToString(), 
                                   dataRow.Cell(ARTIST_COL).Value.ToString(), 
                                   dataRow.Cell(YEAR_COL).Value.ToString(),
                                   dataRow.Cell(SELECTED_COL).Value.ToString().Trim().ToLower() == "yes"));
    }
}

songs = songs.OrderByDescending(s => s.IncludeInBook).ThenBy(s => s.Year).ToList();
Console.WriteLine($"{songs.Count} songs found");
File.WriteAllText("badFiles.txt", "");

char currentLetter = 'A';
int currentIndex = 0;
foreach (SongInfo s in songs)
{
    s.BookID = $"{currentLetter}{currentIndex}";
    Directory.CreateDirectory(@$"Output\{currentLetter}");
    Console.WriteLine($"Processing {s.Name} - {s.Artist} ({s.ID}.mp3) as {currentLetter}{currentIndex}");
    string inputFile = @$".\repository\{s.ID}.mp3";
    string outputFileWav = @$".\Output\{currentLetter}\{currentIndex}.RAW";
    string outputFileTxt = @$".\Output\{currentLetter}\{currentIndex}.TXT";
    
    var proc = new Process
    {
        StartInfo = new ProcessStartInfo
        {
            FileName = "ffmpeg.exe",
            Arguments = @$"-i ""{Path.GetFullPath(inputFile)}"" -y -f s16le -ac 1 -ar 44100 -acodec pcm_s16le -hide_banner -loglevel error ""{Path.GetFullPath(outputFileWav)}""",
            UseShellExecute = false,
            RedirectStandardError = true,
            CreateNoWindow = true,
            WorkingDirectory = "."
        }
    };

    proc.Start();
    proc.WaitForExit();
    string error = proc.StandardError.ReadToEnd().Trim();
    if (error.Length > 0)
    {
        Console.WriteLine(error);
        File.AppendAllText("badFiles.txt", $"{s.ID} - {s.Name} - {s.Artist}{Environment.NewLine}");
    }

    File.WriteAllText(outputFileTxt, $"{s.Name.Trim()}");
    
    currentIndex++;
    if (currentIndex == 10)
    {
        currentIndex = 0;
        currentLetter++;
    }
}

PDFGenerator.CreateBooklet("booklet.pdf", songs.Where(s => s.IncludeInBook));

Process p = new() { StartInfo = new ProcessStartInfo("booklet.pdf") { UseShellExecute = true } };
p.Start();