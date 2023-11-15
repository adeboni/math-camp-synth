namespace MP3toWAVConverter
{
    public class SongInfo
    {
        public string ID { get; set; }
        public string Name { get; set; }
        public string Artist { get; set; }
        public string Year { get; set; }
        public string? BookID { get; set; }
        public bool IncludeInBook { get; set; }

        public SongInfo(string id, string name, string artist, string year, bool includeInBook)
        {
            ID = id;
            Name = name;
            Artist = artist;
            Year = year;
            IncludeInBook = includeInBook;
        }
    }
}
